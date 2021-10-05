﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ztecSerialApp
{
  public partial class Form1 : Form, kr.co.ztec.util.ISerial
  {

    kr.co.ztec.util.Serial _serial;

    public Form1()
    {
      InitializeComponent();
      InitComponent();
    }

    public void InitComponent()
    {
      this.m_btnOpen.Text = "OPEN";
      this.m_btnWrite.Text = "WRITE";
      this.m_btnClear.Text = "CLEAR";

      _serial = new kr.co.ztec.util.Serial(this);
    }

    public void Read(byte[] b, int sz)
    {
      m_txtRead.Text = Encoding.Default.GetString(b);
    }

    private void m_btnOpen_Click(object sender, EventArgs e)
    {
      if (this.m_btnOpen.Text == "OPEN")
      {
        if (this._serial.Open(this.m_txtPort.Text, this.m_txtBaud.Text) > 0)
        {
          this.m_btnOpen.Text = "CLOSE";
        }
      }
      else
      {
        if (this._serial.Close() > 0)
        {
          this.m_btnOpen.Text = "OPEN";
        }
      }
    }

    private void m_btnWrite_Click(object sender, EventArgs e)
    {

    }

    private void m_btnClear_Click(object sender, EventArgs e)
    {

    }

  }
}