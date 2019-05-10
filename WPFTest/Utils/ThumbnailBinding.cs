using Dizz;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Markup;
using System.Windows.Media.Imaging;

namespace WPFTest
{
    [MarkupExtensionReturnType(typeof(BindingExpression))]
    internal class ThumbnailBinding : MarkupExtension
    {
        internal static WeakReference<ThumbnailMan> ThumbMan = new WeakReference<ThumbnailMan>(null);
        internal static WeakReference<TextureLoader> TexLoad = new WeakReference<TextureLoader>(null);

        public PropertyPath Path { get; set; }
        public object Source { get; set; }
        public BindingMode Mode { get; set; } = BindingMode.TwoWay;

        public ThumbnailBinding() { }
        public ThumbnailBinding(string path)
        {
            Path = new PropertyPath(path);
        }

        public override object ProvideValue(IServiceProvider serviceProvider)
        {
            var provideValueTargetService = (IProvideValueTarget)serviceProvider.GetService(typeof(IProvideValueTarget));
            if (provideValueTargetService == null)
                return null;
            if (provideValueTargetService.TargetObject != null &&
                provideValueTargetService.TargetObject.GetType().FullName == "System.Windows.SharedDp")
                return this;
            if (!(provideValueTargetService.TargetObject is FrameworkElement targetObject) ||
                !(provideValueTargetService.TargetProperty is DependencyProperty targetProperty))
                return null;

            object tex2img(object x)
            {
                if ((x is TexHolder holder) && ThumbMan.TryGetTarget(out ThumbnailMan thumbMan))
                    return thumbMan.GetThumbnailAsync(holder);
                else
                    return null;
            }
            object img2tex(object x)
            {
                if (TexLoad.TryGetTarget(out TextureLoader texLoader))
                {
                    switch (x)
                    {
                    case string fname:
                        return texLoader.LoadTextureAsync(fname, TexLoadType.Color);
                    case BitmapSource bmp:
                        return texLoader.LoadTextureAsync(bmp, TexLoadType.Color);
                    }
                }
                return null;
            }
            ProxyBinder binder;
            if (Source == null)
            {
                binder = new ProxyBinder(Path, Mode, tex2img, img2tex);
                BindingOperations.SetBinding(binder, ProxyBinder.DataSourceProperty, new Binding
                {
                    Path = new PropertyPath(FrameworkElement.DataContextProperty),
                    Source = targetObject,
                    Mode = Mode
                });
            }
            else
            {
                binder = new ProxyBinder(new Binding
                {
                    Path = Path,
                    Source = Source,
                    Mode = Mode
                }, tex2img, img2tex);
            }

            var retBinding = binder.GetDestBinding();
            retBinding.ConverterParameter = binder; // keep binder object alive
            return retBinding.ProvideValue(serviceProvider);
        }
    }
}
