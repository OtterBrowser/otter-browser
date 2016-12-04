var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement);

if (element)
{
	var formElement = element.closest('form');

	if (formElement)
	{
		var inputElements = formElement.querySelectorAll('input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]');
		var query = [];

		for (var i = 0; i < inputElements.length; ++i)
		{
			if (inputElements[i].name == '')
			{
				continue;
			}

			if (inputElements[i].tagName.toLowerCase() == 'select')
			{
				var optionElements = inputElements[i].querySelectorAll('option:checked');

				for (var j = 0; j < optionElements.length; ++j)
				{
					query.push(inputElements[i].name + '=' + encodeURIComponent(optionElements[j].value));
				}
			}
			else
			{
				var type = (inputElements[i].type ? inputElements[i].type.toLowerCase() : '');

				if (type == 'image' || type == 'submit' || ((type == 'checkbox' || type == 'radio') && !inputElements[i].checked))
				{
					continue;
				}

				query.push(inputElements[i].name + '=' + ((inputElements[i] == element) ? '{searchTerms}' : encodeURIComponent(inputElements[i].value)));
			}
		}

		var result = {
			url: formElement.action,
			method: formElement.method,
			enctype: formElement.enctype,
			query: query.join('&')
		}

		result;
	}
}
