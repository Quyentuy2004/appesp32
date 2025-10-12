package com.example.testdangky

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import com.example.testdangky.databinding.ActivityChitietthietbiBinding
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase

class Chitietthietbi : AppCompatActivity() {
    private lateinit var binding: ActivityChitietthietbiBinding
    private lateinit var database: DatabaseReference

    private var uid: String = ""
    private var deviceName: String = ""
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        //khởi tạo viewbinding
        binding = ActivityChitietthietbiBinding.inflate(layoutInflater)
        setContentView(binding.root)
        // Nhận dữ liệu từ Intent
        uid = intent.getStringExtra("uid") ?: ""
        deviceName = intent.getStringExtra("name") ?: ""
        Log.d("Intent","$uid")
        Log.d("Intent","$deviceName")
        binding.btnDelete.setOnClickListener {
         database=FirebaseDatabase.getInstance().getReference(uid).child(deviceName)
       database.removeValue()
            database=FirebaseDatabase.getInstance().getReference("User").child(uid).child(deviceName)
            database.removeValue().addOnCompleteListener { task-> if (task.isSuccessful) {
                val intent = Intent(this, manhinhketnoi::class.java)
                startActivity(intent)
                finish()
            } else {
                Log.e("Firebase", "Xóa thất bại: ${task.exception}")
            } }

        }
binding.btnWar.setOnClickListener {

    // Tạo Intent sang Activity khác
    val intent = Intent(this, manhinhcanhbao::class.java)

    // Gửi kèm thông tin
    intent.putExtra("uid", uid)
    intent.putExtra("name", deviceName)

    // Chạy activity mới
    startActivity(intent)
}
        binding.btnHis.setOnClickListener {

            // Tạo Intent sang Activity khác
            val intent = Intent(this, manhinhHistory::class.java)

            // Gửi kèm thông tin
            intent.putExtra("uid", uid)
            intent.putExtra("name", deviceName)

            // Chạy activity mới
            startActivity(intent)
        }
        binding.btnSetting.setOnClickListener {

            // Tạo Intent sang Activity khác
            val intent = Intent(this, manhinhSettings::class.java)

            // Gửi kèm thông tin
            intent.putExtra("uid", uid)
            intent.putExtra("name", deviceName)

            // Chạy activity mới
            startActivity(intent)
        }



    }
}