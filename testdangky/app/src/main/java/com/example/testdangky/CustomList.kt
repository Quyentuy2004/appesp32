package com.example.testdangky

import android.annotation.SuppressLint
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Switch
import android.widget.TextView
import com.example.testdangky.R.id.txtname

class CustomList (val activity: manhinhketnoi, val list:List<giaodienhienthingoai>): ArrayAdapter<giaodienhienthingoai>(activity,R.layout.giaodientbi){
    // alt+insert
    override fun getCount(): Int {
        return list.size// ve len view voi tat ca dong cua list
    }


    override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
        val contexs = activity.layoutInflater
        val rowView = contexs.inflate(R.layout.giaodientbi,parent,false)

        val hum= rowView.findViewById<TextView>(R.id.txthum)
        val temp= rowView.findViewById<TextView>(R.id.txttemp)
        val name= rowView.findViewById<TextView>(R.id.txtname)
        val online=rowView.findViewById<Switch>(R.id.btnOnline)


        hum.text=list[position].hum
        temp.text=list[position].temp
        name.text=list[position].name
        online.isChecked = list[position].Online
//        if (online==true){
//
//        }
        return rowView
    }
}