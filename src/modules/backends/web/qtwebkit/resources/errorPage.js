(function(window)
{
	var addSslErrorExceptionButton = document.querySelector('button.addSslErrorException');

	if (addSslErrorExceptionButton)
	{
		addSslErrorExceptionButton.addEventListener('click', function(event)
		{
			var request = new XMLHttpRequest();
			request.addEventListener('load', function(event)
			{
				window.location.reload(true);
			});
			request.addEventListener('error', function(event)
			{
				window.location.reload(true);
			});
			request.open('GET', '/otter-message', true);
			request.setRequestHeader('X-Otter-Token', '%1');
			request.setRequestHeader('X-Otter-Type', 'add-ssl-error-exception');
			request.setRequestHeader('X-Otter-Data', btoa(JSON.stringify({ url: window.location.href, digest: '%2' })));
			request.send(null);
		});
	}

	var addContentBlockingExceptionButton = document.querySelector('button.addContentBlockingException');

	if (addContentBlockingExceptionButton)
	{
		addContentBlockingExceptionButton.addEventListener('click', function(event)
		{
			var request = new XMLHttpRequest();
			request.addEventListener('load', function(event)
			{
				window.location.reload(true);
			});
			request.addEventListener('error', function(event)
			{
				window.location.reload(true);
			});
			request.open('GET', '/otter-message', true);
			request.setRequestHeader('X-Otter-Token', '%1');
			request.setRequestHeader('X-Otter-Type', 'add-content-blocking-exception');
			request.setRequestHeader('X-Otter-Data', btoa(JSON.stringify({ url: window.location.href })));
			request.send(null);
		});
	}

	var advancedButton = document.querySelector('button.advanced');

	if (advancedButton)
	{
		advancedButton.addEventListener('click', function(event)
		{
			var descriptionElement = document.querySelector('div.description');

			if (descriptionElement)
			{
				descriptionElement.style.display = ((descriptionElement.style.display == 'block') ? 'none' : 'block');
			}
		});
	}

	var goBackButton = document.querySelector('button.goBack');

	if (goBackButton)
	{
		goBackButton.addEventListener('click', function(event)
		{
			if (%3)
			{
				window.history.back();
			}
			else
			{
				window.location.href = 'about:blank';
			}
		});
	}

	var reloadPageButton = document.querySelector('button.reloadPage');

	if (reloadPageButton)
	{
		reloadPageButton.addEventListener('click', function(event)
		{
			window.location.reload(true);
		});
	}
})(window);
