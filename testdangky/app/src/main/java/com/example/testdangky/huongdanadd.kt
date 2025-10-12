package com.example.testdangky

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.example.testdangky.databinding.ActivityHuongdanaddBinding
import com.example.testdangky.databinding.ActivityKetnoiEsp32Binding

private lateinit var binding: ActivityHuongdanaddBinding
class huongdanadd : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        //khởi tạo viewbinding
        binding = ActivityHuongdanaddBinding.inflate(layoutInflater)
        setContentView(binding.root)
        binding.btnXacnhan.setOnClickListener {
            startActivity(Intent(this, ketnoiEsp32::class.java))
        }
    }
}