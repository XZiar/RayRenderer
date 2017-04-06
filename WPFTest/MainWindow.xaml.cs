using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace WPFTest
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            Main.test.resize(glMain.ClientSize.Width & 0xffc0, glMain.ClientSize.Height & 0xffc0);
            //this.SizeChanged += (o, e) => { glMain.Size = new System.Drawing.Size{ Width = (int)e.NewSize.Width,Height = (int)e.NewSize.Height}; };
            this.Closed += (o, e) => { Main.test.Dispose(); Main.test = null; };

            glMain.Draw += Main.test.draw;
            glMain.Resize += (o, e) => { Main.test.resize(e.Width & 0xffc0, e.Height & 0xffc0); };
            //oglv.KeyDown += onKeyBoard;
            //glMain.KeyAction += OnKeyAction;
            //glMain.MouseAction += OnMouse;
        }
    }
}
