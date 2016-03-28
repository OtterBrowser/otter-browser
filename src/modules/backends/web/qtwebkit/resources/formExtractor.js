(function(window)
{
	function attachFormListener(form)
	{
		form.addEventListener('submit', function(event)
		{
			var inputs = form.querySelectorAll('input:not([disabled])[name]');
			var fields = {};
			var passwords = [];

			for (var i = 0; i < inputs.length; ++i)
			{
				if (inputs[i].type == 'email' || inputs[i].type == 'password' || inputs[i].type == 'text')
				{
					fields[inputs[i].name] = inputs[i].value;

					if (inputs[i].type == 'password')
					{
						passwords.push(inputs[i].name);
					}
				}
			}

			if (passwords.length > 0)
			{
				var request = new XMLHttpRequest();
				request.open('GET', '/otter-password', true);
				request.setRequestHeader('X-Otter-Token', '%1');
				request.setRequestHeader('X-Otter-Data', btoa(JSON.stringify({ url: window.location.href, fields: fields, passwords: passwords })));
				request.send(null);
			}
		});
	}

	for (var i = 0; i < document.forms.length; ++i)
	{
		attachFormListener(document.forms[i]);
	}
})(window);
