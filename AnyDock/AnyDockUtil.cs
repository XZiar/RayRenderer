﻿using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;

namespace AnyDock
{
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
    internal class DragData
    {
        internal readonly UIElement Element;
        internal readonly AnyDockPanel Panel;
        internal readonly bool AllowDrag;
        internal DragData(AnyDockTabLabel source) : this((UIElement)source.DataContext)
        { }
        internal DragData(UIElement source)
        {
            Element = source;
            Panel = AnyDockManager.GetParentDock(Element);
            AllowDrag = AnyDockManager.GetAllowDrag(Element);
        }
    }

    public class AnyDockManager
    {
        private static readonly DependencyProperty ParentDockProperty = DependencyProperty.RegisterAttached(
            "ParentDock",
            typeof(AnyDockPanel),
            typeof(AnyDockManager),
            new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.AffectsRender));
        internal static void SetParentDock(UIElement element, AnyDockPanel value) => element.SetValue(ParentDockProperty, value);
        internal static AnyDockPanel GetParentDock(UIElement element) => element == null ? null : element.GetValue(ParentDockProperty) as AnyDockPanel;

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

        public static readonly DependencyProperty ClosableProperty = DependencyProperty.RegisterAttached(
            "Closable",
            typeof(bool),
            typeof(AnyDockManager),
            new FrameworkPropertyMetadata(true, FrameworkPropertyMetadataOptions.None));
        public static bool GetClosable(UIElement element) => (bool)element.GetValue(ClosableProperty);
        public static void SetClosable(UIElement element, bool value) => element.SetValue(ClosableProperty, value);

        public class TabClosingEventArgs : RoutedEventArgs
        {
            public readonly AnyDockPanel ParentPanel;
            public bool ShouldClose = false;
            internal TabClosingEventArgs(UIElement element, AnyDockPanel panel) : base(ClosingEvent, element) { ParentPanel = panel; }
        }
        public delegate void TabClosingEventHandler(UIElement sender, TabClosingEventArgs args);

        public static readonly RoutedEvent ClosingEvent = EventManager.RegisterRoutedEvent(
            "Closing",
            RoutingStrategy.Direct,
            typeof(TabClosingEventHandler),
            typeof(AnyDockManager));
        public static void AddClosingHandler(UIElement element, TabClosingEventHandler handler) =>
            element.AddHandler(ClosingEvent, handler);
        public static void RemoveClosingHandler(UIElement element, TabClosingEventHandler handler) =>
            element.RemoveHandler(ClosingEvent, handler);
    }
}