using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using NUnit.Framework;
using System.Net.Mail;
using System.Net;
using System.Threading;

namespace DevSmtp.Tests
{
	[TestFixture]
	public class MainTests
	{
		[Test]
		public void SendMailTest()
		{
			SendEmail(
				"me@somehost.com",
				"someone@somehost.com",
				"Hi there!",
				"It's the body, no more.",
				true,
				IPAddress.Loopback.ToString()
			);
		}

		private void SendEmail(string from, string to, string subject, string body, bool isHtml, string host)
		{
			var message = new MailMessage(from, to);

			message.Body = body;
			message.Subject = subject;

			message.BodyEncoding = Encoding.UTF8;
			message.SubjectEncoding = Encoding.UTF8;

			var client = new SmtpClient(host);

			message.IsBodyHtml = isHtml;

			client.Send(message);
		}

		static void Main()
		{
			const int threads = 10;
			// wait a second. we should allow our server to start.
			Thread.Sleep(1000);

			var tests = new MainTests();

			// Run one time
			tests.SendMailTest();

			int entered = threads;
			int left = threads;
			var mreEntered = new ManualResetEvent(false);
			var mreLeft = new ManualResetEvent(false);				

			// Run in a cycle
			for (int i = 0; i < threads; i++)
			{
				ThreadPool.QueueUserWorkItem((state) =>
				{
					if (Interlocked.Decrement(ref entered) == 0)
					{
						mreEntered.Set();
					}
					else
					{
						mreEntered.WaitOne();
					}

					tests.SendMailTest();

					if (Interlocked.Decrement(ref left) == 0)
					{
						mreLeft.Set();
					}
				});
			}

			mreLeft.WaitOne();
		}
	}
}
