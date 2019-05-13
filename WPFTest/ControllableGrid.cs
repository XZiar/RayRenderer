using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Common;
using XZiar.Util.Collections;

namespace WPFTest
{

    [ContentProperty(nameof(Children))]
    public class ControllableGrid : ContentControl
    {
        private static readonly ResourceDictionary ResDict;
        private static readonly ItemsPanelTemplate ItemsPanelTemplate;
        static ControllableGrid()
        {
            ResDict = new ResourceDictionary { Source = new Uri("WPFTest;component/ControllableGrid.res.xaml", UriKind.RelativeOrAbsolute) };
            ItemsPanelTemplate = (ItemsPanelTemplate)ResDict["ItemsPanelTemplate"];
        }

        private class ControlGroupsControl : ItemsControl
        {
            private readonly ControllableGrid Upper;
            internal ControlGroupsControl(ControllableGrid upper)
            {
                Upper = upper;
                ItemsPanel = ItemsPanelTemplate;
            }
            protected override DependencyObject GetContainerForItemOverride()
            {
                return new ControllableGroup(Upper.BindingForeground, Upper.BindingBackground);
            }

            protected override void PrepareContainerForItemOverride(DependencyObject element, object item)
            {
                var control = (ControllableGroup)element;
                control.DataContext = item;
                control.GeneratingControl += OnGenerateControl;
            }

            private void OnGenerateControl(ControllableGroup sender, Controllable control, ControlItem item, ref FrameworkElement element)
            {
                Upper.GeneratingControl?.Invoke(Upper, control, item, ref element);
            }
        }


        public static readonly DependencyProperty ItemCategoryProperty = DependencyProperty.RegisterAttached(
            "ItemCategory",
            typeof(string),
            typeof(ControllableGrid),
            new FrameworkPropertyMetadata("", FrameworkPropertyMetadataOptions.AffectsRender));
        public static void SetItemCategory(UIElement element, string value)
        {
            element.SetValue(ItemCategoryProperty, value);
        }
        public static string GetItemCategory(UIElement element)
        {
            return element.GetValue(ItemCategoryProperty) as string;
        }

        public static readonly DependencyProperty ItemNameProperty = DependencyProperty.RegisterAttached(
            "ItemName",
            typeof(string),
            typeof(ControllableGrid),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        public static void SetItemName(UIElement element, string value)
        {
            element.SetValue(ItemNameProperty, value);
        }
        public static string GetItemName(UIElement element)
        {
            return element.GetValue(ItemNameProperty) as string;
        }

        public static readonly DependencyProperty ItemDescriptionProperty = DependencyProperty.RegisterAttached(
            "ItemDescription",
            typeof(string),
            typeof(ControllableGrid),
            new FrameworkPropertyMetadata("", FrameworkPropertyMetadataOptions.AffectsRender));
        public static void SetItemDescription(UIElement element, string value)
        {
            element.SetValue(ItemDescriptionProperty, value);
        }
        public static string GetItemDescription(UIElement element)
        {
            return element.GetValue(ItemDescriptionProperty) as string;
        }

        public ObservableCollection<FrameworkElement> Children { get; } = new ObservableCollection<FrameworkElement>();

        //public static readonly DependencyProperty CategoriesProperty = DependencyProperty.Register(
        //    nameof(Categories),
        //    typeof(Dictionary<string, string>),
        //    typeof(ControllableGrid),
        //    new PropertyMetadata());
        //public Dictionary<string, string> Categories
        //{
        //    get { return (Dictionary<string, string>)GetValue(CategoriesProperty); }
        //    set { SetValue(CategoriesProperty, value); }
        //}

        public delegate void GenerateControlEventHandler(ControllableGrid sender, Controllable control, ControlItem item, ref FrameworkElement element);
        public event GenerateControlEventHandler GeneratingControl;
        public ObservableSet<string> ExceptIds { get; } = new ObservableSet<string>();

        private readonly ItemsControl stkMain;

        private readonly Dictionary<string, string> Categories = new Dictionary<string, string>();
        private readonly List<Grid> CategoryGrids = new List<Grid>();


        public ControllableGrid()
        {
            stkMain = new ControlGroupsControl(this);
            Content = stkMain;
            ExceptIds.CollectionChanged += ExceptIdsChanged;
            DataContextChanged += OnDataContextChanged;
            BindingForeground = new Binding
            {
                Source = this,
                Path = new PropertyPath("Foreground"),
                Mode = BindingMode.OneWay
            };
            BindingBackground = new Binding
            {
                Source = this,
                Path = new PropertyPath("Background"),
                Mode = BindingMode.OneWay
            };
            BindingBorderBrush = new Binding
            {
                Source = this,
                Path = new PropertyPath("BorderBrush"),
                Mode = BindingMode.OneWay
            };
        }

        private void ExceptIdsChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            Refresh();
        }

        private readonly Binding BindingForeground;
        private readonly Binding BindingBackground;
        private readonly Binding BindingBorderBrush;
        private static readonly BrushConverter BrushConv = new BrushConverter();
        private static readonly SolidColorBrush BrushBorder = BrushConv.ConvertFromString("#FFABABAB") as SolidColorBrush;

        private void OnDataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            Refresh();
        }

        internal class ControlPack
        {
            public readonly string Name;
            public readonly string Description;
            public readonly ControlItem Item;
            public readonly FrameworkElement Control;
            public ControlPack(ControlItem item)
            {
                Name = item.Name; Description = item.Description; Item = item; Control = null;
            }
            public ControlPack(FrameworkElement control)
            {
                Name = GetItemName(control); Description = GetItemDescription(control); Item = null; Control = control;
            }
        }
        internal class ControlGroup
        {
            public readonly Controllable Control;
            public string CategoryName { get; }
            public readonly string CategoryId;
            public readonly List<ControlPack> Controls = new List<ControlPack>();
            public ControlGroup(Controllable control, string id, string name)
            {
                Control = control; CategoryId = id; CategoryName = name;
            }
        }
        private void Refresh()
        {
            foreach (var grid in CategoryGrids)
                grid.Children.Clear();
            CategoryGrids.Clear();

            if (!(DataContext is Controllable control))
            {
                stkMain.DataContext = null;
                stkMain.ItemsSource = null;
                return;
            }

            stkMain.DataContext = control;
            foreach (var element in Children)
                element.DataContext = control;

            var ctrlGroups = control.Items.Where(x => !ExceptIds.Contains(x.Id))
                .Select(x => x.Category)
                .Concat(Children.Select(x => GetItemCategory(x)))
                .Distinct()
                .ToDictionary(x => x, x =>
                {
                    if (!Categories.TryGetValue(x, out var catName) &&
                        !control.Categories.TryGetValue(x, out catName))
                            catName = x;
                    return new ControlGroup(control, x, catName);
                });
            foreach (var item in control.Items.Where(x => !ExceptIds.Contains(x.Id)))
            {
                ctrlGroups[item.Category].Controls.Add(new ControlPack(item));
            }
            foreach (var ctrl in Children)
            {
                ctrlGroups[GetItemCategory(ctrl)].Controls.Add(new ControlPack(ctrl));
            }

            stkMain.ItemsSource = ctrlGroups.Values;
        }

    }
}
