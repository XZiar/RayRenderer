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
    /// DrawablePage.xaml 的交互逻辑
    /// </summary>
    public partial class DrawablePage : ContentControl
    {
        public DrawablePage()
        {
            InitializeComponent();
            matGrid.CtrlGrid.ExceptIds.Add("Color");
        }

        private void texture_Drop(object sender, DragEventArgs e)
        {
            string fname = (e.Data.GetData(System.Windows.Forms.DataFormats.FileDrop) as Array).GetValue(0).ToString();
            try
            {
                //WaitingCount++;
                var img = new BitmapImage(new Uri(fname, UriKind.Absolute));
                var imgCtrl = ((sender as Border).Child as StackPanel).Children.OfType<Image>().First();
                imgCtrl.Source = img;
                //var tex = await Core.TexLoader.LoadTextureAsync(fname, TexLoadType.Color);
                //var mat = (sender as Border).DataContext as PBRMaterial;
                //mat.DiffuseMap = tex;
            }
            catch (Exception ex)
            {
                new TextDialog(ex).ShowDialog();
            }
            finally
            {
                //WaitingCount--;
            }
        }

        private void texture_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effects = DragDropEffects.Link; e.Handled = true;
            }
        }
    }
}
