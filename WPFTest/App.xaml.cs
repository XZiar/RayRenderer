using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace WPFTest
{
    /// <summary>
    /// App.xaml 的交互逻辑
    /// </summary>
    public partial class App : Application
    {
        static App()
        {
            //AppDomain.CurrentDomain.AssemblyResolve += ResolveDLL;
            AppDomain.CurrentDomain.UnhandledException += HandleExceptions;
            Console.WriteLine(Environment.Is64BitProcess ? "current in x64" : "current in x86");
        }

        private static void HandleExceptions(object sender, UnhandledExceptionEventArgs e)
        {
            var ex = e.ExceptionObject as Exception;
            Console.WriteLine($"Unexpected Exception {ex.GetType()}\n{ex.Message}\n");
            new TextDialog(ex).ShowDialog();
            throw ex;
        }

        private static Assembly ResolveDLL(object sender, ResolveEventArgs args)
        {
            string[] fields = args.Name.Split(',');
            string name = fields[0];
            string culture = fields[2];
            // failing to ignore queries for satellite resource assemblies or using [assembly: NeutralResourcesLanguage("en-US", UltimateResourceFallbackLocation.MainAssembly)] 
            // in AssemblyInfo.cs will crash the program on non en-US based system cultures.
            if (name.EndsWith(".resources") && !culture.EndsWith("neutral"))
                return null;

            /* the actual assembly resolver */
            Console.WriteLine($"resolve Assembly {args.Name}");
            var dllname = name + (Environment.Is64BitProcess ? ".x64.dll" : ".x86.dll");
            return Assembly.LoadFrom(dllname);
        }

        private void AppStartup(object sender, StartupEventArgs e)
        {
            System.Windows.Forms.Application.SetHighDpiMode(System.Windows.Forms.HighDpiMode.PerMonitorV2);
            var idx = Array.FindIndex(e.Args, arg => arg.StartsWith("--renderdoc"));
            if (idx != -1)
            {
                var arg = e.Args[idx];
                var path = arg.Length > 11 ? arg.Substring(12) : "";
                Dizz.RenderCore.InjectRenderDoc(path);
                Dizz.RenderCore.InitGLEnvironment();
            }
        }
    }
}
