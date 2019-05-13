using Common;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Xceed.Wpf.Toolkit;
using static XZiar.Util.BindingHelper;

namespace WPFTest
{
    /// <summary>
    /// ControllableGroup.xaml 的交互逻辑
    /// </summary>
    internal partial class ControllableGroup : Expander
    {
        private static readonly TwoWayValueConvertor ConvertorForceUShort = TwoWayValueConvertor.From(Convert.ToUInt16);
        private static readonly TwoWayValueConvertor ConvertorForceInt = TwoWayValueConvertor.From(Convert.ToInt32);
        private static readonly TwoWayValueConvertor ConvertorForceSingle = TwoWayValueConvertor.From(Convert.ToSingle);
        private static readonly BrushConverter BrushConv = new BrushConverter();
        private static readonly SolidColorBrush BrushBorder = BrushConv.ConvertFromString("#FFABABAB") as SolidColorBrush;

        public delegate void GenerateControlEventHandler(ControllableGroup sender, Controllable control, ControlItem item, ref FrameworkElement element);
        public event GenerateControlEventHandler GeneratingControl;

        private readonly Binding BindingForeground;
        private readonly Binding BindingBackground;

        public ControllableGroup(Binding fg, Binding bg)
        {
            DataContextChanged += OnDataContextChanged;
            BindingForeground = fg;
            BindingBackground = bg;
            SetBinding(ForegroundProperty, fg);
            SetBinding(BackgroundProperty, bg);
            InitializeComponent();
        }

        private void OnDataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            GridMain.RowDefinitions.Clear();
            if (!(DataContext is ControllableGrid.ControlGroup data))
                return;
            PrepareGrid(data);
        }

        private void SetFGBGBinding(FrameworkElement element)
        {
            switch (element)
            {
            case Panel panel:
                panel.SetBinding(Panel.BackgroundProperty, BindingBackground);
                break;
            case TextBlock tblock:
                tblock.SetBinding(TextBlock.BackgroundProperty, BindingBackground);
                tblock.SetBinding(TextBlock.ForegroundProperty, BindingForeground);
                break;
            case Control control:
                if (!(control is CheckBox))
                {
                    control.SetBinding(Control.BackgroundProperty, BindingBackground);
                    control.SetBinding(Control.ForegroundProperty, BindingForeground);
                    //control.SetBinding(Control.BorderBrushProperty, BindingBorderBrush);
                }
                break;
            }
        }
        private void PrepareGrid(ControllableGrid.ControlGroup groupData)
        {
            for (var i = 0; i < groupData.Controls.Count(); ++i)
                GridMain.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            int idx = 0, idxEnd = groupData.Controls.Count() - 1;
            foreach (var pack in groupData.Controls)
            {
                var realControl = pack.Control;
                if (realControl == null)
                {
                    realControl = OnGeneratingControl(groupData.Control, pack.Item);
                    SetFGBGBinding(realControl);
                }
                realControl.Margin = new Thickness(4, 4, 4, 4);
                bool setTooltip = !string.IsNullOrWhiteSpace(pack.Description);
                if (string.IsNullOrWhiteSpace(pack.Name))
                {
                    if (setTooltip)
                        realControl.ToolTip = pack.Description;
                    Grid.SetRow(realControl, idxEnd); Grid.SetColumn(realControl, 0); Grid.SetColumnSpan(realControl, 3);
                    GridMain.Children.Add(realControl);
                    idxEnd--;
                }
                else
                {
                    var label = new TextBlock()
                    {
                        Text = pack.Name,
                        Padding = new Thickness(4, 0, 4, 0),
                        VerticalAlignment = VerticalAlignment.Center,
                        HorizontalAlignment = HorizontalAlignment.Center
                    };
                    if (setTooltip)
                        label.ToolTip = pack.Description;
                    label.SetBinding(VisibilityProperty, new Binding
                    {
                        Source = realControl,
                        Path = new PropertyPath(VisibilityProperty),
                        Mode = BindingMode.OneWay
                    });
                    SetFGBGBinding(label);

                    Grid.SetRow(label, idx); Grid.SetColumn(label, 0);
                    GridMain.Children.Add(label);
                    Grid.SetRow(realControl, idx); Grid.SetColumn(realControl, 2);
                    GridMain.Children.Add(realControl);
                    idx++;
                }
            }
            if (idx > 0)
            {
                Grid.SetRowSpan(GridSplit, idx);
                GridSplit.Visibility = Visibility.Visible;
            }
            else
                GridSplit.Visibility = Visibility.Collapsed;
        }

        public static Binding CreateControlItemBinding(Controllable control, ControlItem item, IValueConverter convertor = null)
        {
            return new Binding(item.Id)
            {
                Source = control,
                Mode = item.Access.HasFlag(ControlItem.PropAccess.Write) ? BindingMode.TwoWay : BindingMode.OneWay,
                Converter = convertor
            };
        }

        private FrameworkElement OnGeneratingControl(Controllable control, ControlItem item)
        {
            FrameworkElement result = null;
            GeneratingControl?.Invoke(this, control, item, ref result);
            if (result != null) return result;
            if (item.Type == ControlItem.PropType.Enum)
            {
                var cbox = new ComboBox
                {
                    ItemsSource = item.Cookie as string[]
                };
                cbox.SetBinding(ComboBox.SelectedItemProperty, CreateControlItemBinding(control, item));
                return cbox;
            }
            if (item.ValType == typeof(string))
            {
                if (item.Type == ControlItem.PropType.LongText)
                {
                    var btn = new Button { Content = "查看" };
                    btn.Click += (o, e) =>
                    {
                        control.DoGetMember(item.Id, out object txt);
                        new TextDialog(txt as string, $"{control.ToString()} --- {item.Name}").Show();
                        e.Handled = true;
                    };
                    return btn;
                }
                {
                    var tbox = new TextBox
                    {
                        VerticalAlignment = VerticalAlignment.Center
                    };
                    tbox.SetBinding(TextBox.TextProperty, CreateControlItemBinding(control, item));
                    tbox.IsReadOnly = !item.Access.HasFlag(ControlItem.PropAccess.Write);
                    return tbox;
                }
            }
            if (item.ValType == typeof(Vector2))
            {
                var limit = (Vector2)item.Cookie;
                var slider = new RangeSlider
                {
                    AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                    AutoToolTipPrecision = 3,
                    Minimum = limit.X,
                    Maximum = limit.Y
                };
                slider.SetBinding(RangeSlider.LowerValueProperty,
                    CreateControlItemBinding(control, item, new TwoWayValueConvertor(
                        o => ((Vector2)o).X,
                        o => new Vector2(Convert.ToSingle(o), (float)slider.HigherValue))
                        ));
                slider.SetBinding(RangeSlider.HigherValueProperty,
                    CreateControlItemBinding(control, item, new TwoWayValueConvertor(
                        o => ((Vector2)o).Y,
                        o => new Vector2((float)slider.LowerValue, Convert.ToSingle(o)))
                        ));
                return slider;
            }
            if (item.ValType == typeof(Color))
            {
                var clrPicker = new ColorPicker()
                {
                    ColorMode = ColorMode.ColorCanvas
                };
                clrPicker.SetBinding(ColorPicker.SelectedColorProperty, CreateControlItemBinding(control, item));
                return clrPicker;
            }
            if (item.ValType == typeof(float))
            {
                if (item.Cookie == null)
                {
                    var spiner = new DoubleUpDown { FormatString = "F2", Increment = 0.1 };
                    spiner.SetBinding(DoubleUpDown.ValueProperty, CreateControlItemBinding(control, item, ConvertorForceSingle));
                    return spiner;
                }
                else
                {
                    var limit = (Vector2)item.Cookie;
                    var slider = new Slider
                    {
                        AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                        AutoToolTipPrecision = 3,
                        Minimum = limit.X,
                        Maximum = limit.Y
                    };
                    slider.SetBinding(Slider.ValueProperty, CreateControlItemBinding(control, item, ConvertorForceSingle));
                    return slider;
                }
            }
            if (item.ValType == typeof(ushort))
            {
                if (item.Cookie == null)
                {
                    var spiner = new IntegerUpDown { Increment = 1 };
                    spiner.SetBinding(IntegerUpDown.ValueProperty, CreateControlItemBinding(control, item, ConvertorForceUShort));
                    return spiner;
                }
                else
                {
                    dynamic limit = item.Cookie;
                    var slider = new Slider
                    {
                        AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                        Minimum = limit.Item1,
                        Maximum = limit.Item2
                    };
                    slider.SetBinding(Slider.ValueProperty, CreateControlItemBinding(control, item, ConvertorForceUShort));
                    return slider;
                }
            }
            if (item.ValType == typeof(int))
            {
                if (item.Cookie == null)
                {
                    var spiner = new IntegerUpDown { Increment = 1 };
                    spiner.SetBinding(IntegerUpDown.ValueProperty, CreateControlItemBinding(control, item));
                    return spiner;
                }
                else
                {
                    dynamic limit = item.Cookie;
                    var slider = new Slider
                    {
                        AutoToolTipPlacement = AutoToolTipPlacement.TopLeft,
                        Minimum = limit.Item1,
                        Maximum = limit.Item2
                    };
                    slider.SetBinding(Slider.ValueProperty, CreateControlItemBinding(control, item, ConvertorForceInt));
                    return slider;
                }
            }
            if (item.ValType == typeof(bool))
            {
                var ckBox = new CheckBox
                {
                    VerticalAlignment = VerticalAlignment.Center,
                    HorizontalAlignment = HorizontalAlignment.Center
                };
                ckBox.SetBinding(CheckBox.IsCheckedProperty, CreateControlItemBinding(control, item));
                return ckBox;
            }
            {
                var tblock = new TextBlock
                {
                    VerticalAlignment = VerticalAlignment.Center,
                    HorizontalAlignment = HorizontalAlignment.Center
                };
                tblock.SetBinding(TextBlock.TextProperty, CreateControlItemBinding(control, item));
                return tblock;
            }
        }
    }
}
