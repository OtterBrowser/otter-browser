(function(window)
{
	let addSslErrorExceptionButton = document.querySelector('button.addSslErrorException');

	if (addSslErrorExceptionButton)
	{
		addSslErrorExceptionButton.addEventListener('click', function(event)
		{
			let request = new XMLHttpRequest();
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
			request.setRequestHeader('X-Otter-Action', 'add-ssl-error-exception');
			request.setRequestHeader('X-Otter-Data', btoa(JSON.stringify({ url: window.location.href, digest: '%2' })));
			request.send(null);
		});
	}

	let addContentBlockingExceptionButton = document.querySelector('button.addContentBlockingException');

	if (addContentBlockingExceptionButton)
	{
		addContentBlockingExceptionButton.addEventListener('click', function(event)
		{
			let request = new XMLHttpRequest();
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
			request.setRequestHeader('X-Otter-Action', 'add-content-blocking-exception');
			request.setRequestHeader('X-Otter-Data', btoa(JSON.stringify({ url: window.location.href })));
			request.send(null);
		});
	}

	let advancedButton = document.querySelector('button.advanced');

	if (advancedButton)
	{
		advancedButton.addEventListener('click', function(event)
		{
			let descriptionElement = document.querySelector('div.description');

			if (descriptionElement)
			{
				descriptionElement.style.display = ((descriptionElement.style.display == 'block') ? 'none' : 'block');
			}
		});
	}

	let goBackButton = document.querySelector('button.goBack');

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

	let reloadPageButton = document.querySelector('button.reloadPage');

	if (reloadPageButton)
	{
		reloadPageButton.addEventListener('click', function(event)
		{
			window.location.reload(true);
		});
	}
})(window);
