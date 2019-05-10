using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;

namespace AnyDock
{
    internal class CollapseIfEmptyConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (int)value == 0 ? Visibility.Collapsed : Visibility.Visible;
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    internal class CollapseIfNullConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value == null ? Visibility.Collapsed : Visibility.Visible;
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    internal class CollapseIfFalseConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (value as bool?) == true ? Visibility.Visible : Visibility.Collapsed;
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
    internal class CollapseIfTrueConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (value as bool?) == true ? Visibility.Collapsed : Visibility.Visible;
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    internal class TabStripAngleConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            switch ((Dock)value)
            {
            case Dock.Left: return 270.0;
            case Dock.Right: return 90.0;
            default: return 0.0;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    internal class GridLengthOnOriConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (Orientation)value == (Orientation)parameter ? new GridLength(0, GridUnitType.Star) : new GridLength(1.5);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    static class Helper
    {
        internal static IEnumerable<T> DeledItems<T>(this NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Reset ||
                e.Action == NotifyCollectionChangedAction.Remove ||
                e.Action == NotifyCollectionChangedAction.Replace)
                if (e.OldItems != null)
                    return e.OldItems.Cast<T>();
            return Enumerable.Empty<T>();
        }
        internal static IEnumerable<T> AddedItems<T>(this NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Add ||
                e.Action == NotifyCollectionChangedAction.Replace)
                if (e.NewItems != null)
                    return e.NewItems.Cast<T>();
            return Enumerable.Empty<T>();
        }
    }

    public static class AnyDockManager
    {
        public static readonly DependencyProperty PageNameProperty = DependencyProperty.RegisterAttached(
            "PageName",
            typeof(string),
            typeof(AnyDockManager),
            new FrameworkPropertyMetadata("Page", FrameworkPropertyMetadataOptions.AffectsRender));
        public static string GetPageName(UIElement element) => element.GetValue(PageNameProperty) as string;
        public static void SetPageName(UIElement element, string value) => element.SetValue(PageNameProperty, value);

        public static readonly DependencyProperty PageIconProperty = DependencyProperty.RegisterAttached(
            "PageIcon",
            typeof(ImageSource),
            typeof(AnyDockManager),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        public static ImageSource GetPageIcon(UIElement element) => element.GetValue(PageIconProperty) as ImageSource;
        public static void SetPageIcon(UIElement element, ImageSource value) => element.SetValue(PageIconProperty, value);

        public static readonly DependencyProperty AllowDragProperty = DependencyProperty.RegisterAttached(
            "AllowDrag",
            typeof(bool),
            typeof(AnyDockManager),
            new FrameworkPropertyMetadata(true, FrameworkPropertyMetadataOptions.None));
        public static bool GetAllowDrag(UIElement element) => (bool)element.GetValue(AllowDragProperty);
        public static void SetAllowDrag(UIElement element, bool value) => element.SetValue(AllowDragProperty, value);

        public static readonly DependencyProperty CanCloseProperty = DependencyProperty.RegisterAttached(
            "CanClose",
            typeof(bool),
            typeof(AnyDockManager),
            new FrameworkPropertyMetadata(true, FrameworkPropertyMetadataOptions.None));
        public static bool GetCanClose(UIElement element) => (bool)element.GetValue(CanCloseProperty);
        public static void SetCanClose(UIElement element, bool value) => element.SetValue(CanCloseProperty, value);

        public class TabCloseEventArgs : RoutedEventArgs
        {
            public readonly UIElement TargetElement;
            public bool ShouldClose = true;
            internal TabCloseEventArgs(UIElement element) : base(ClosingEvent) { TargetElement = element; }
        }
        public delegate void TabCloseEventHandler(UIElement sender, TabCloseEventArgs args);

        public static readonly RoutedEvent ClosingEvent = EventManager.RegisterRoutedEvent(
            "Closing",
            RoutingStrategy.Direct,
            typeof(TabCloseEventHandler),
            typeof(AnyDockManager));
        public static void AddClosingHandler(UIElement element, TabCloseEventHandler handler) =>
            element.AddHandler(ClosingEvent, handler);
        public static void RemoveClosingHandler(UIElement element, TabCloseEventHandler handler) =>
            element.RemoveHandler(ClosingEvent, handler);

        internal static readonly RoutedEvent RemovedEvent = EventManager.RegisterRoutedEvent(
            "Removed",
            RoutingStrategy.Direct,
            typeof(RoutedEventHandler),
            typeof(AnyDockManager));
        internal static void AddRemovedHandler(UIElement element, RoutedEventHandler handler) =>
            element.AddHandler(RemovedEvent, handler);
        internal static void RemoveRemovedHandler(UIElement element, RoutedEventHandler handler) =>
            element.RemoveHandler(RemovedEvent, handler);
        internal static void RaiseRemovedEvent(UIElement element)
        {
            element.RaiseEvent(new RoutedEventArgs(RemovedEvent));
        }

    }
}
