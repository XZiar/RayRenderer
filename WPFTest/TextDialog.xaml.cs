using System;
using System.Collections.Generic;
using System.Drawing;
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
using XZiar.WPFControl;

namespace WPFTest
{
    /// <summary>
    /// TextDialog.xaml 的交互逻辑
    /// </summary>
    public partial class TextDialog : Window
    {
        static ImageSource iconError = SystemIcons.Error.ToImageSource();
        public TextDialog(Exception ex)
        {
            InitializeComponent();
            Icon = iconError;
            var sb = new StringBuilder($"{ex.GetType().Name}\t{ex.Message}\n\t{ex.StackTrace}\n");
            for (var iex = ex.InnerException; iex != null; iex = iex.InnerException)
                sb.Append($"\nCaused by:\n{iex.Message}\n\t{iex.StackTrace}\n");
            extxt.Text = sb.ToString();
            Title = ex.GetType().ToString();
        }

        public TextDialog(string content, string title = "")
        {
            InitializeComponent();
            Title = title;
            extxt.Text = content;
        }

        private void confirm_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}
