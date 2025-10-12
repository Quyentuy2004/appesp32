package com.example.testdangky

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.text.TextUtils
import android.view.View
import android.widget.Toast
import com.example.testdangky.databinding.ActivityMainBinding
import com.google.firebase.auth.FirebaseAuth

private lateinit var binding: ActivityMainBinding
class MainActivity : AppCompatActivity() {
    private lateinit var fAuth: FirebaseAuth
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        //khởi tạo viewbinding
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        fAuth = FirebaseAuth.getInstance()

        if (fAuth.currentUser != null) {
            startActivity(Intent(this, manhinhketnoi::class.java))
           finish()
        }
         binding.txtchuyendangky.setOnClickListener {
           var intent1= Intent(/* packageContext = */ this, /* cls = */ dangnhap::class.java)
            startActivity(intent1)
         }
        binding.btndangnhap.setOnClickListener { xulydangnhap() }
    }

    private fun xulydangnhap() {
        val email= binding.edtEmaildangnhap.text.toString().trim()
        val password= binding.edtPassworddangnhap.text.toString().trim()
        if(TextUtils.isEmpty(email)){
            binding.edtEmaildangnhap.setError("Khong duoc de trong Email")
            return
        }
        if(TextUtils.isEmpty(password)){
            binding.edtPassworddangnhap.setError("Khong duoc de trong Password")
            return
        }

        if(password.length<6){
            binding.edtPassworddangnhap.setError("Password phai co it nhat 6 ky tu")
            binding.edtPassworddangnhap.setText("")
            return
        }
        binding.progressBar2.visibility=View.VISIBLE
        fAuth.signInWithEmailAndPassword(email,password).addOnCompleteListener(this) {  task ->
            if (task.isSuccessful) {
                Toast.makeText(this, "Đăng nhap thành công", Toast.LENGTH_SHORT).show()
                startActivity(Intent(this, manhinhketnoi::class.java))
                finish()
            } else {
                Toast.makeText(this, "Lỗi: ${task.exception?.message}", Toast.LENGTH_SHORT).show()
                binding.progressBar2.visibility = View.INVISIBLE
            } }
    }
}