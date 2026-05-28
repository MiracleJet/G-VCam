package com.mj.gvcam

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

data class AppUiState(
    val isServerRunning: Boolean = false,
    val isCameraActive: Boolean = false,
    val frameCount: Long = 0,
    val clientCount: Int = 0,
    val port: Int = 8080,
    val resolution: String = "1920x1080",
    val quality: Int = 55
)

interface CameraController {
    suspend fun start(): Boolean
    suspend fun stop()
}

class GVCamViewModel : ViewModel() {

    private val _uiState = MutableStateFlow(AppUiState())
    val uiState: StateFlow<AppUiState> = _uiState.asStateFlow()

    val server = MjpegServer(8080)
    var cameraController: CameraController? = null
    private var statsJob: Job? = null

    fun onFrame(jpeg: ByteArray) {
        server.sendFrame(jpeg)
    }

    fun start() {
        viewModelScope.launch {
            val cameraOk = cameraController?.start() ?: true
            if (!cameraOk) return@launch

            server.start()
            _uiState.value = _uiState.value.copy(
                isServerRunning = true, isCameraActive = true
            )
            statsJob = viewModelScope.launch {
                while (true) {
                    _uiState.value = _uiState.value.copy(
                        frameCount = server.frameCount,
                        clientCount = server.clientCount
                    )
                    delay(1000)
                }
            }
        }
    }

    fun stop() {
        statsJob?.cancel()
        statsJob = null
        server.stop()
        _uiState.value = _uiState.value.copy(
            isServerRunning = false, isCameraActive = false
        )
        viewModelScope.launch {
            cameraController?.stop()
        }
    }

    override fun onCleared() {
        super.onCleared()
        stop()
    }
}
