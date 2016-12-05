var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement);

if (element)
{
	function createUrl(url)
	{
		var element = document.createElement('a');
		element.href = url;

		return element.href;
	}

	var formElement = element.closest('form');

	if (formElement)
	{
		var result = {
			url: createUrl(formElement.action),
			method: formElement.method,
			enctype: formElement.enctype,
			query: ''
		}
		var tagName = element.tagName.toLowerCase();
		var searchTermsElement = null;
		var inputElements = Array.from(formElement.querySelectorAll('button:not([disabled])[name][type="submit"], input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]'));
		var query = [];

		if (tagName !== 'select')
		{
			var type = (element.type ? element.type.toLowerCase() : '');

			if (inputElements.indexOf(element) < 0 && (type == 'image' || type == 'submit'))
			{
				inputElements.push(element);
			}

			if (tagName == 'textarea' || type == 'email' || type == 'password' || type == 'search' || type == 'tel' || type == 'text' || type == 'url')
			{
				searchTermsElement = element;
			}
		}

		if (!searchTermsElement)
		{
			searchTermsElement = formElement.querySelector('input:not([disabled])[name][type="search"]');
		}

		for (var i = 0; i < inputElements.length; ++i)
		{
			var tagName = inputElements[i].tagName.toLowerCase();

			if (tagName !== 'select')
			{
				var type = (inputElements[i].type ? inputElements[i].type.toLowerCase() : '');
				var isSubmit = (type == 'image' || type == 'submit');

				if ((isSubmit && inputElements[i] != element) || ((type == 'checkbox' || type == 'radio') && !inputElements[i].checked))
				{
					continue;
				}

				if (isSubmit && inputElements[i] == element)
				{
					if (inputElements[i].hasAttribute('formaction'))
					{
						result.url = createUrl(inputElements[i].getAttribute('formaction'));
					}

					if (inputElements[i].hasAttribute('formenctype'))
					{
						result.enctype = inputElements[i].getAttribute('formenctype');
					}

					if (inputElements[i].hasAttribute('formmethod'))
					{
						result.method = inputElements[i].getAttribute('formmethod');
					}
				}

				if (!isSubmit && !searchTermsElement && type !== 'hidden')
				{
					searchTermsElement = inputElements[i];
				}

				if (inputElements[i].name !== '')
				{
					query.push(inputElements[i].name + '=' + ((inputElements[i] == searchTermsElement) ? '{searchTerms}' : encodeURIComponent((tagName == 'button') ? inputElements[i].innerHTML : inputElements[i].value)));
				}
			}
			else if (inputElements[i].name !== '')
			{
				var optionElements = inputElements[i].querySelectorAll('option:checked');

				for (var j = 0; j < optionElements.length; ++j)
				{
					query.push(inputElements[i].name + '=' + encodeURIComponent(optionElements[j].value));
				}
			}
		}

		result.query = query.join('&');

		result;
	}
}
