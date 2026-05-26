package com.mj.gvcam

interface Platform {
    val name: String
}

expect fun getPlatform(): Platform