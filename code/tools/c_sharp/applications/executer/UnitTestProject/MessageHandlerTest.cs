using message_handler;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Windows.Forms;

namespace UnitTestProject
{
    
    
    /// <summary>
    ///This is a test class for MessageHandlerTest and is intended
    ///to contain all MessageHandlerTest Unit Tests
    ///</summary>
	[TestClass()]
	public class MessageHandlerTest
	{


		private TestContext testContextInstance;

		/// <summary>
		///Gets or sets the test context which provides
		///information about and functionality for the current test run.
		///</summary>
		public TestContext TestContext
		{
			get
			{
				return testContextInstance;
			}
			set
			{
				testContextInstance = value;
			}
		}

		#region Additional test attributes
		// 
		//You can use the following additional attributes as you write your tests:
		//
		//Use ClassInitialize to run code before running the first test in the class
		//[ClassInitialize()]
		//public static void MyClassInitialize(TestContext testContext)
		//{
		//}
		//
		//Use ClassCleanup to run code after all tests in a class have run
		//[ClassCleanup()]
		//public static void MyClassCleanup()
		//{
		//}
		//
		//Use TestInitialize to run code before running each test
		//[TestInitialize()]
		//public void MyTestInitialize()
		//{
		//}
		//
		//Use TestCleanup to run code after each test has run
		//[TestCleanup()]
		//public void MyTestCleanup()
		//{
		//}
		//
		#endregion


		/// <summary>
		///A test for Show
		///</summary>
		[TestMethod()]
		public void ShowTest()
		{
			IWin32Window owner = null; // TODO: Initialize to an appropriate value
			string text = string.Empty; // TODO: Initialize to an appropriate value
			string caption = string.Empty; // TODO: Initialize to an appropriate value
			MessageBoxButtons buttons = new MessageBoxButtons(); // TODO: Initialize to an appropriate value
			MessageBoxIcon icon = new MessageBoxIcon(); // TODO: Initialize to an appropriate value
			MessageBoxDefaultButton defaultButton = new MessageBoxDefaultButton(); // TODO: Initialize to an appropriate value
			DialogResult expected = new DialogResult(); // TODO: Initialize to an appropriate value
			DialogResult actual;
			actual = MessageHandler.Show(owner, text, caption, buttons, icon, defaultButton);
			Assert.AreEqual(expected, actual);
			Assert.Inconclusive("Verify the correctness of this test method.");
		}
	}
}
