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

        setContent {
            val viewModel = viewModel<GVCamViewModel>(viewModelStoreOwner = this@MainActivity)
            val context = this@MainActivity
            val lifecycleOwner = LocalLifecycleOwner.current

            LaunchedEffect(Unit) {
                if (ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA)
                    == PackageManager.PERMISSION_GRANTED
                ) {
                    cameraSource = AndroidCameraSource(
                        context = context,
                        lifecycleOwner = lifecycleOwner,
                        previewView = previewView,
                        onFrame = viewModel.onFrameReady
                    )
                    cameraSource?.start()
                    viewModel.startServer()
                    viewModel.setCameraActive(true)
                } else {
                    requestPermissionLauncher.launch(Manifest.permission.CAMERA)
                }
            }

            DisposableEffect(Unit) {
                onDispose {
                    cameraSource?.shutdown()
                    viewModel.stopAll()
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
