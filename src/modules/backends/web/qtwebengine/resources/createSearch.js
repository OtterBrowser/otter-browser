var element = ((%1 >= 0) ? document.elementFromPoint((%1 + window.scrollX), (%2 + window.scrollX)) : document.activeElement);

if (element)
{
	var form = element.closest('form');

	if (form)
	{
		var inputs = form.querySelectorAll('input:not([disabled])[name], select:not([disabled])[name], textarea:not([disabled])[name]');
		var query = [];

		for (var i = 0; i < inputs.length; ++i)
		{
			var value = '';

			if (inputs[i].tagName.toLowerCase() == 'select')
			{
				value = inputs[i].options[inputs[i].selectedIndex].value;
			}
			else
			{
				if (inputs[i].type == 'submit' || ((inputs[i].type == 'checkbox' || inputs[i].type == 'radio') && !inputs[i].checked))
				{
					continue;
				}

				value = inputs[i].value;
			}

			query.push(inputs[i].name + '=' + ((inputs[i] == element) ? '{searchTerms}' : encodeURIComponent(value)));
		}

		var result = {
			url: form.action,
			method: form.method,
			enctype: form.enctype,
			query: query.join('&')
		}

		result;
	}
}
