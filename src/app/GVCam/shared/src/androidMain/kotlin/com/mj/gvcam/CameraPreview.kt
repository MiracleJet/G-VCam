package com.mj.gvcam

import androidx.camera.view.PreviewView
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.viewinterop.AndroidView

@Composable
fun CameraPreview(
    previewView: PreviewView,
    modifier: Modifier = Modifier
) {
    AndroidView(
        factory = { previewView },
        modifier = modifier
    )
}
