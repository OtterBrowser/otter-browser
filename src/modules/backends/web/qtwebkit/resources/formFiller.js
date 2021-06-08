(function(window)
{
	const fields = %1;

	for (let i = 0; i < document.forms.length; ++i)
	{
		let isMatched = true;

		for (let j = 0; j < fields.length; ++j)
		{
			if (document.forms[i].querySelector('input[name=' + fields[j].name + ']' + ((fields[j].type == 'password') ? '[type=password]' : '')) == null)
			{
				isMatched = false;

				break;
			}
		}

		if (isMatched)
		{
			for (let j = 0; j < fields.length; ++j)
			{
				let element = document.forms[i].querySelector('input[name=' + fields[j].name + ']' + ((fields[j].type == 'password') ? '[type=password]' : ''));

				if (element !== null)
				{
					element.value = fields[j].value;
				}
			}
		}
	}
})(window);
