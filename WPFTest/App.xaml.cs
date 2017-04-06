using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Reflection;
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
            AppDomain.CurrentDomain.AssemblyResolve += ResolveDLL;
            Console.WriteLine(Environment.Is64BitProcess ? "current in x64" : "current in x86");
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
    }
}
