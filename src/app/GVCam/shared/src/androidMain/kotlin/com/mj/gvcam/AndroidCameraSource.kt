package com.mj.gvcam

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.ImageFormat
import android.graphics.Rect
import android.graphics.YuvImage
import android.util.Log
import android.util.Size
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import java.io.ByteArrayOutputStream
import java.nio.ByteBuffer
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class AndroidCameraSource(
    private val context: Context,
    private val lifecycleOwner: LifecycleOwner,
    private val previewView: PreviewView,
    private val onFrame: (ByteArray) -> Unit
) {
    private var cameraProvider: ProcessCameraProvider? = null
    private var camera: Camera? = null
    private var lensFacing = CameraSelector.LENS_FACING_BACK
    private val cameraExecutor: ExecutorService = Executors.newFixedThreadPool(2)

    var targetResolution = Size(1920, 1080)
    var quality: Int = 55

    private var nv21Buffer: ByteArray? = null
    private val jpegOutStream = ThreadLocal.withInitial { ByteArrayOutputStream(200_000) }

    fun start() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(context)
        cameraProviderFuture.addListener({
            cameraProvider = cameraProviderFuture.get()
            bindUseCases()
        }, ContextCompat.getMainExecutor(context))
    }

    private fun bindUseCases() {
        val provider = cameraProvider ?: return
        val selector = CameraSelector.Builder().requireLensFacing(lensFacing).build()

        val preview = Preview.Builder()
            .setTargetResolution(targetResolution)
            .build()
            .also { it.setSurfaceProvider(previewView.surfaceProvider) }

        val analysis = ImageAnalysis.Builder()
            .setTargetResolution(targetResolution)
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .build()
            .also {
                it.setAnalyzer(cameraExecutor) { image ->
                    val jpeg = image.toJpeg()
                    image.close()
                    if (jpeg != null) onFrame(jpeg)
                }
            }

        try {
            provider.unbindAll()
            camera = provider.bindToLifecycle(lifecycleOwner, selector, preview, analysis)
        } catch (e: Exception) {
            Log.e("GVCam", "Camera bind failed", e)
        }
    }

    @SuppressLint("UnsafeOptInUsageError")
    private fun ImageProxy.toJpeg(): ByteArray? {
        return try {
            val w = width; val h = height
            val size = w * h * 3 / 2
            if (nv21Buffer == null || nv21Buffer!!.size != size)
                nv21Buffer = ByteArray(size)
            val nv21 = nv21Buffer!!

            yuv420ToNv21(this, nv21)
            val yuv = YuvImage(nv21, ImageFormat.NV21, w, h, null)
            val out = jpegOutStream.get()!!
            out.reset()
            yuv.compressToJpeg(Rect(0, 0, w, h), quality, out)
            out.toByteArray()
        } catch (e: Exception) {
            Log.e("GVCam", "JPEG conversion failed", e)
            null
        }
    }

    private fun yuv420ToNv21(image: ImageProxy, nv21: ByteArray) {
        val w = image.width; val h = image.height
        val planes = image.planes
        val yBuf = planes[0].buffer
        val uBuf = planes[1].buffer
        val vBuf = planes[2].buffer

        val yRowStride = planes[0].rowStride
        if (yRowStride == w) {
            yBuf.get(nv21, 0, w * h)
        } else {
            for (row in 0 until h) {
                yBuf.position(row * yRowStride)
                yBuf.get(nv21, row * w, w)
            }
        }

        val vRowStride = planes[2].rowStride
        val vPixelStride = planes[2].pixelStride
        val uRowStride = planes[1].rowStride
        val uPixelStride = planes[1].pixelStride
        var pos = w * h

        if (vPixelStride == 2 && vBuf.remaining() == (w * h / 2 - 1)) {
            vBuf.get(nv21, pos, vBuf.remaining())
        } else {
            for (row in 0 until h / 2)
                for (col in 0 until w / 2) {
                    nv21[pos++] = vBuf.get(row * vRowStride + col * vPixelStride)
                    nv21[pos++] = uBuf.get(row * uRowStride + col * uPixelStride)
                }
        }
    }

    fun switchCamera() {
        lensFacing = if (lensFacing == CameraSelector.LENS_FACING_BACK)
            CameraSelector.LENS_FACING_FRONT
        else CameraSelector.LENS_FACING_BACK
        bindUseCases()
    }

    fun shutdown() {
        cameraExecutor.shutdown()
        cameraProvider?.unbindAll()
    }
}
