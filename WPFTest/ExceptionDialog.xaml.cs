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

namespace WPFTest
{
    /// <summary>
    /// ExceptionDialog.xaml 的交互逻辑
    /// </summary>
    public partial class ExceptionDialog : Window
    {
        public ExceptionDialog(Exception ex)
        {
            InitializeComponent();
            var sb = new StringBuilder($"{ex.GetType().Name}\t{ex.Message}\n\t{ex.StackTrace}\n");
            for (var iex = ex.InnerException; iex != null; iex = iex.InnerException)
                sb.Append($"\nCaused by:\n{iex.Message}\n\t{iex.StackTrace}\n");
            extxt.Text = sb.ToString();
            Title = ex.GetType().ToString();
        }

        private void confirm_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}
