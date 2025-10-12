package com.example.testdangky

import android.content.Intent
import android.os.Bundle
import android.text.TextUtils
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.example.testdangky.databinding.ActivityDangnhapBinding
import com.google.firebase.auth.FirebaseAuth

private lateinit var binding: ActivityDangnhapBinding
class dangnhap : AppCompatActivity() {
    private lateinit var fAuth: FirebaseAuth
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //khởi tạo viewbinding
        binding = ActivityDangnhapBinding.inflate(layoutInflater)
        setContentView(binding.root)
        fAuth= FirebaseAuth.getInstance()


        if (fAuth.currentUser != null) {
            startActivity(Intent(this, manhinhketnoi::class.java))
            finish()
        }
        binding.txtchuyendangnhap.setOnClickListener {
          var intent2= Intent(this,MainActivity::class.java)
           startActivity(intent2)
        }

        binding.button2.setOnClickListener { xulidangky() }
    }

    private fun xulidangky() {
        val email= binding.edtEmaildangky.text.toString().trim()
        val password1= binding.editTextNumberPassword.text.toString().trim()
        val password2= binding.editTextNumberPassword2.text.toString().trim()

        if(TextUtils.isEmpty(email)){
            binding.edtEmaildangky.setError("Khong duoc de trong Email")
            return
        }
        if(TextUtils.isEmpty(password1)){
            binding.editTextNumberPassword.setError("Khong duoc de trong Password")
            return
        }
        if(TextUtils.isEmpty(password2)){
            binding.editTextNumberPassword2.setError("Ban can nhap lai Password")
            return
        }
        if(password1.length<6){
            binding.editTextNumberPassword.setError("Password phai co it nhat 6 ky tu")
            binding.editTextNumberPassword.setText("")
            return
        }

        if(password1!=password2){
            binding.editTextNumberPassword2.setError("Password nhap lai sai")
            binding.editTextNumberPassword2.setText("")
            return
        }
        binding.progressBar.visibility = View.VISIBLE
        fAuth.createUserWithEmailAndPassword(email,password1).addOnCompleteListener(this){
                task ->
            if (task.isSuccessful) {
                Toast.makeText(this, "Đăng ký thành công", Toast.LENGTH_SHORT).show()
                startActivity(Intent(this, manhinhketnoi::class.java))
                finish()
            } else {
                Toast.makeText(this, "Lỗi: ${task.exception?.message}", Toast.LENGTH_SHORT).show()
                binding.progressBar.visibility = View.INVISIBLE
            }
        }
    }
}