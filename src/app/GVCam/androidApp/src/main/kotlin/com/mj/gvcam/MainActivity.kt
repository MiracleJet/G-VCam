package com.mj.gvcam

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.camera.view.PreviewView
import androidx.compose.runtime.*
import androidx.core.content.ContextCompat
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.viewmodel.compose.viewModel

class MainActivity : ComponentActivity() {

    private lateinit var previewView: PreviewView
    private var cameraSource: AndroidCameraSource? = null
    private var permissionChecked = false

    private val requestPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
            if (granted) recreate()
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)

        previewView = PreviewView(this).also {
            it.implementationMode = PreviewView.ImplementationMode.COMPATIBLE
        }

        if (!permissionChecked) {
            permissionChecked = true
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED
            ) {
                requestPermissionLauncher.launch(Manifest.permission.CAMERA)
            }
        }

        setContent {
            val viewModel = viewModel<GVCamViewModel>(viewModelStoreOwner = this@MainActivity)
            val context = this@MainActivity
            val lifecycleOwner = LocalLifecycleOwner.current

            LaunchedEffect(Unit) {
                viewModel.cameraController = object : CameraController {
                    override suspend fun start(): Boolean {
                        if (cameraSource != null) return true
                        cameraSource = AndroidCameraSource(
                            context = context,
                            lifecycleOwner = lifecycleOwner,
                            previewView = previewView,
                            onFrame = { viewModel.onFrame(it) }
                        )
                        cameraSource?.start()
                        return true
                    }

                    override suspend fun stop() {
                        cameraSource?.shutdown()
                        cameraSource = null
                    }
                }
            }

            App(
                viewModel = viewModel,
                previewContent = {
                    CameraPreview(previewView)
                }
            )
        }
    }
}
