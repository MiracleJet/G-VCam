package com.mj.gvcam

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.compose.viewModel

@Composable
fun App(
    viewModel: GVCamViewModel = viewModel { GVCamViewModel() },
    previewContent: @Composable () -> Unit = {}
) {
    val state by viewModel.uiState.collectAsStateWithLifecycle()

    MaterialTheme {
        Column(
            modifier = Modifier.fillMaxSize().padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = "GVCam",
                style = MaterialTheme.typography.headlineMedium
            )

            Spacer(Modifier.height(8.dp))

            // Camera preview area
            Box(
                modifier = Modifier.fillMaxWidth().weight(1f),
                contentAlignment = Alignment.Center
            ) {
                if (state.isCameraActive) {
                    previewContent()
                } else {
                    Text("Camera not active", color = MaterialTheme.colorScheme.onSurfaceVariant)
                }
            }

            Spacer(Modifier.height(8.dp))

            // Status
            Card(modifier = Modifier.fillMaxWidth()) {
                Column(modifier = Modifier.padding(12.dp)) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Server")
                        Text(
                            if (state.isServerRunning) "ON ●" else "OFF",
                            color = if (state.isServerRunning) MaterialTheme.colorScheme.primary
                                    else MaterialTheme.colorScheme.error
                        )
                    }
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Port")
                        Text(state.port.toString())
                    }
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Frames sent")
                        Text(state.frameCount.toString())
                    }
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Clients")
                        Text(state.clientCount.toString())
                    }
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Resolution")
                        Text(state.resolution)
                    }
                }
            }

            Spacer(Modifier.height(12.dp))

            // Controls
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Button(
                    onClick = { viewModel.startServer() },
                    enabled = !state.isServerRunning,
                    modifier = Modifier.weight(1f)
                ) { Text("Start Server") }

                Button(
                    onClick = { viewModel.stopAll() },
                    enabled = state.isServerRunning,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = MaterialTheme.colorScheme.error
                    )
                ) { Text("Stop") }
            }
        }
    }
}
