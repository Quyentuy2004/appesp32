package com.example.testdangky

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import com.example.testdangky.databinding.ActivityKetnoiEsp32Binding
import org.json.JSONArray
import com.androidnetworking.AndroidNetworking
import com.androidnetworking.common.Priority
import com.androidnetworking.error.ANError
import com.androidnetworking.interfaces.JSONArrayRequestListener
import com.androidnetworking.interfaces.StringRequestListener
import com.google.firebase.auth.FirebaseAuth

private lateinit var binding: ActivityKetnoiEsp32Binding
class ketnoiEsp32 : AppCompatActivity() {
    private lateinit var fAuth: FirebaseAuth
    private val list = mutableListOf<String>()
    private var SSID:String ="" // dùng chung cho mọi hàm
    private var Password:String ="" // dùng chung cho mọi ham
    private var uid:String =""
    private var Name:String =""
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //khởi tạo viewbinding
        binding = ActivityKetnoiEsp32Binding.inflate(layoutInflater)
        setContentView(binding.root)
        fAuth = FirebaseAuth.getInstance()
        val user = fAuth.currentUser ?: return
         uid = user.uid.toString()
      Toast.makeText(this@ketnoiEsp32,"Lấy thành công uid: $uid",Toast.LENGTH_SHORT ).show()
        // Khởi tạo mặc định
        AndroidNetworking.initialize(applicationContext)
        binding.btnscan.setOnClickListener { scanwifi() }
        ngheSSID()
        binding.btnSave.setOnClickListener { savewifi() }
        binding.btnRestart.setOnClickListener { restartEsp32() }
    }

    private fun ngheSSID() {
        binding.spnSSID.onItemSelectedListener=object : AdapterView.OnItemSelectedListener{
            override fun onItemSelected(
                parent: AdapterView<*>?,
                view: View?,
                position: Int,
                id: Long
            ) {
                SSID=list[position].toString()
                Toast.makeText(this@ketnoiEsp32,"ban chon "+list[position], Toast.LENGTH_SHORT).show()
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {

            }
        }
    }

    private fun restartEsp32() {
        AndroidNetworking.post("http://192.168.4.1/reStart")
            .setTag("test")
            .setPriority(Priority.HIGH)
            .build()
            .getAsString(/* requestListener = */ object : com.androidnetworking.interfaces.StringRequestListener {
                override fun onResponse(response: String) {
                    Toast.makeText(this@ketnoiEsp32, response, Toast.LENGTH_SHORT).show()
                    startActivity(Intent(this@ketnoiEsp32, manhinhketnoi::class.java))
                    finish()
                }

                override fun onError(anError: ANError) {
                    Toast.makeText(this@ketnoiEsp32, anError.errorBody, Toast.LENGTH_SHORT).show()
                }
            })


    }

    private fun savewifi() {
        Password=binding.edtPassword.text.toString()
        Name=binding.edtName.text.toString()
        AndroidNetworking.post("http://192.168.4.1/saveWifi")
            .addBodyParameter("ssid",SSID )
            .addBodyParameter("pass",Password )
            .addBodyParameter("uid",uid )
            .addBodyParameter("name",Name )
            .setTag("test")
            .setPriority(Priority.HIGH)
            .build()
            .getAsString(/* requestListener = */ object : com.androidnetworking.interfaces.StringRequestListener {
                override fun onResponse(response: String) {
                    Toast.makeText(this@ketnoiEsp32, response, Toast.LENGTH_SHORT).show()
                }

                override fun onError(anError: ANError) {
                    Toast.makeText(this@ketnoiEsp32, anError.errorBody, Toast.LENGTH_SHORT).show()
                }
            })

    }


    private fun scanwifi() {
binding.progressBar3.visibility=View.VISIBLE
        AndroidNetworking.get("http://192.168.4.1/scanWifi")
            .setPriority(Priority.HIGH)
            .build()
            .getAsJSONArray(object : JSONArrayRequestListener {
                override fun onResponse(response: JSONArray?) {
                    // In nguyên JSON ra log
                    Log.d("GET", "Response JSON: $response")
                    list.clear()  // xoá dữ liệu cũ
                    list.add("Chon Wifi")
                    val setSSID = mutableSetOf<String>() // dùng set để loại trùng
                    // In từng SSID ra log
                    for (i in 0 until response!!.length()) {
                        val ssid = response.getString(i).trim()
                        if (ssid.isNotEmpty()) {      // bỏ SSID rỗng
                            setSSID.add(ssid)         // tự động loại bỏ trùng
                        }
                    }

                    list.addAll(setSSID)  // đưa dữ liệu set về list
                    val adt= ArrayAdapter(this@ketnoiEsp32, R.layout.spiner_xml,R.id.txtspn,list.toList())
                    adt.setDropDownViewResource(R.layout.spiner_xml)
                    binding.spnSSID.adapter=adt
                    binding.progressBar3.visibility=View.INVISIBLE
                }

                override fun onError(anError: ANError?) {
                    Log.e("GET", "Error: ${anError?.message}")
                    binding.progressBar3.visibility=View.INVISIBLE
                }
            })
    }
}