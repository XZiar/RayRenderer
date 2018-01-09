using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinFormTest
{
    static class Program
    {
        static Program()
        {
            AppDomain.CurrentDomain.AssemblyResolve += ResolveDLL;
            //AppDomain.CurrentDomain.UnhandledException += HandleExceptions;
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

        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
