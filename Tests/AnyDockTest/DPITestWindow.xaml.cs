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
using System.Windows.Shapes;

namespace AnyDockTest
{
    /// <summary>
    /// DPITestWindow.xaml 的交互逻辑
    /// </summary>
    public partial class DPITestWindow : Window
    {
        public DPITestWindow()
        {
            InitializeComponent();
        }

        private void Rectangle_MouseMove(object sender, MouseEventArgs e)
        {
            var obj = sender as Rectangle;
            var pos = e.GetPosition(obj);
            var screenPos = this.PointToScreen(e.GetPosition(this));
            Info.Text = $"Relative[{pos}]\nScreen[{screenPos}]";
        }
    }
}
