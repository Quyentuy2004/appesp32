package com.example.testdangky

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import com.example.testdangky.databinding.ActivityManhinhketnoiBinding
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener

private lateinit var binding: ActivityManhinhketnoiBinding
class manhinhketnoi : AppCompatActivity() {
    private lateinit var database: DatabaseReference
    lateinit var customAdapter: CustomList
    private lateinit var fAuth: FirebaseAuth
    private var uid:String =""
    private var Checkdathemlist:Int =0
    private var listhum= mutableListOf<String>()
    private var listtemp= mutableListOf<String>()
    private var listonline= mutableListOf<Boolean>()
    private val listtenthietbi = mutableListOf<String>()
    // ✅ Danh sách lưu listener và reference để gỡ sau này
    private val listeners = mutableListOf<ValueEventListener>()
    private val references = mutableListOf<DatabaseReference>()
   private val listhienthigiaodien = mutableListOf<giaodienhienthingoai>()
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //khởi tạo viewbinding
        binding = ActivityManhinhketnoiBinding.inflate(layoutInflater)
        setContentView(binding.root)
        fAuth = FirebaseAuth.getInstance()
        val user = fAuth.currentUser ?: return
        uid = user.uid.toString()
        Toast.makeText(this@manhinhketnoi,"Lấy thành công uid: $uid", Toast.LENGTH_SHORT ).show()


        binding.btnlogout.setOnClickListener {
            FirebaseAuth.getInstance().signOut()
            startActivity(Intent(this, MainActivity::class.java))
            finish()
        }


        binding.btnAdd.setOnClickListener {
            startActivity(Intent(this,huongdanadd::class.java ))
        }

        listThietbi()


        binding.listthietbi.setOnItemClickListener { parent, view, position, id ->
            Log.d("CLICK_TEST", "Bạn đã bấm vào item $position")
            val selectedDevice = listhienthigiaodien[position]

            // Tạo Intent sang Activity khác
            val intent = Intent(this, Chitietthietbi::class.java)

            // Gửi kèm thông tin
            intent.putExtra("uid", uid)
            intent.putExtra("name", selectedDevice.name)

            // Chạy activity mới
            startActivity(intent)
        }

    }

    private fun Capnhatthongso() {
        for ((index, value) in listtenthietbi.withIndex()) {
            Log.d("thiet bi hien co","Element at index $index is $value")
            database = FirebaseDatabase.getInstance().getReference("$uid/$value/Sensor")

            val listener = object : ValueEventListener {
                override fun onDataChange(snapshot: DataSnapshot) {
                    val tempValue = snapshot.child("temp").getValue(Float::class.java)?.toString() ?: "--"
                    val humValue = snapshot.child("hum").getValue(Float::class.java)?.toString() ?: "--"
                    listhienthigiaodien[index].temp = tempValue.toString()
                    listhienthigiaodien[index].hum = humValue.toString()

                    // Cập nhật adapter (chỉ làm mới dòng đó)
                    customAdapter.notifyDataSetChanged()



                }

                override fun onCancelled(error: DatabaseError) {
                    Log.e("FirebaseError", "Lỗi: ${error.message}")
                }
            }

        }

    }

    private fun listThietbi() {

        database= FirebaseDatabase.getInstance().getReference("User/"+ uid)
      val listener = object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                listtenthietbi.clear()
                for (child in snapshot.children) {
                    val key = child.key
                    listtenthietbi.add(key.toString())
                    Log.d("FirebaseKey", "Key: $key")

                }
                listtemp()
            }

            override fun onCancelled(error: DatabaseError) {
                Log.e("FirebaseError", "Lỗi: ${error.message}")
            }
      }
        database.addListenerForSingleValueEvent(listener)
        references.add(database)
        listeners.add(listener)

    }

    private fun listtemp() {
        var demtemp:Int=0
        listtemp.clear()
      for ((index, value) in listtenthietbi.withIndex()){
          database= FirebaseDatabase.getInstance().getReference("$uid/$value/Sensor/temp")
          val listener = object : ValueEventListener {
              override fun onDataChange(snapshot: DataSnapshot) {
                  val tempValue = snapshot.getValue(Float::class.java) ?: 0f
                listtemp.add(tempValue.toString())
                  demtemp+=1
                  if(demtemp==listtenthietbi.size) {
                      themlisthum()
                  }
                  if (Checkdathemlist==1){


                  }
                  Log.d("Firebase", "Nhiệt độ: $tempValue")
              }

              override fun onCancelled(error: DatabaseError) {
                  val tempValue = "--"
                  listtemp.add(tempValue.toString())
                  Log.e("FirebaseError", "Lỗi: ${error.message}")
              }
          }
          database.addValueEventListener(listener)
          references.add(database)
          listeners.add(listener)
      }

    }

    private fun themlisthum() {
        var demhum:Int=0
        listhum.clear()
        for ((index, value) in listtenthietbi.withIndex()){
            database= FirebaseDatabase.getInstance().getReference("$uid/$value/Sensor/hum")
            val listener = object : ValueEventListener {
                override fun onDataChange(snapshot: DataSnapshot) {
                    val humValue = snapshot.getValue(Float::class.java) ?: 0f
                    listhum.add(humValue.toString())
                    demhum+=1
                    if(demhum==listtenthietbi.size){
                    themlistOnline()}
                    if (Checkdathemlist==1){


                    }
                    Log.d("Firebase", "Do am: $humValue")
                }

                override fun onCancelled(error: DatabaseError) {
                    val humValue = "--"
                    listtemp.add(humValue.toString())
                    Log.e("FirebaseError", "Lỗi: ${error.message}")
                }
            }
            database.addValueEventListener(listener)
            references.add(database)
            listeners.add(listener)
        }

    }

    private fun themlistOnline() {
        listonline.clear()
        for ((index, value) in listtenthietbi.withIndex()){

            listonline.add(true)
        }
        themlisthienthi()
    }

    private fun themlisthienthi() {
Checkdathemlist=0
        listhienthigiaodien.clear()
        val list = mutableListOf<giaodienhienthingoai>()
        for ((index, value) in listtenthietbi.withIndex()){
            list.add(giaodienhienthingoai(value, listtemp[index],listhum[index],listonline[index]))

        }
        customAdapter = CustomList(this@manhinhketnoi, list)
        binding.listthietbi.adapter = customAdapter
        Checkdathemlist=1
    }

    override fun onDestroy() {
        super.onDestroy()
        for (i in listeners.indices) {
            references[i].removeEventListener(listeners[i])
        }
        listeners.clear()
        references.clear()
    }
}
