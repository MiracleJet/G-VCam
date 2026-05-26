package com.mj.gvcam

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
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

class GVCamViewModel : ViewModel() {

    private val _uiState = MutableStateFlow(AppUiState())
    val uiState: StateFlow<AppUiState> = _uiState.asStateFlow()

    val server = MjpegServer(8080)

    val onFrameReady: (ByteArray) -> Unit = { jpeg ->
        server.sendFrame(jpeg)
    }

    fun startServer() {
        server.start()
        _uiState.value = _uiState.value.copy(isServerRunning = true)
        viewModelScope.launch {
            while (server.frameCount >= 0) {
                _uiState.value = _uiState.value.copy(
                    frameCount = server.frameCount,
                    clientCount = server.clientCount
                )
                delay(1000)
            }
        }
    }

    fun setCameraActive(active: Boolean) {
        _uiState.value = _uiState.value.copy(isCameraActive = active)
    }

    fun stopAll() {
        server.stop()
        _uiState.value = _uiState.value.copy(isServerRunning = false, isCameraActive = false)
    }

    override fun onCleared() {
        super.onCleared()
        server.stop()
    }
}
