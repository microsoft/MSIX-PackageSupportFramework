//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace GameWithCommonIssues
{
    public partial class PSFSample : Form
    {
        string exePath = "";
        public PSFSample()
        {
            InitializeComponent();
        }

        private void PSFSample_Load(object sender, EventArgs e)
        {
            string result = System.Reflection.Assembly.GetExecutingAssembly().Location;
            int index = result.LastIndexOf("\\");
            exePath = result.Substring(0, index);
            label1.Text = exePath;
            label2.Text = Environment.CurrentDirectory;
        }

        private void buttonReadConfig_Click(object sender, EventArgs e)
        {
            try
            {
                System.IO.File.OpenRead("Config.txt");
                MessageBox.Show("SUCCESS", "App Message");
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString(), "App Message");
            }
        }

        private void buttonWriteLog_Click(object sender, EventArgs e)
        {
            string newFileName = exePath + "\\" + Guid.NewGuid().ToString() + ".log";
            try
            {
                System.IO.File.CreateText(newFileName);
                MessageBox.Show("SUCCESS", "App Message");
            }
            catch (Exception exc)
            {
                MessageBox.Show(exc.ToString(), "App Message");
            }
        }
    }
}
