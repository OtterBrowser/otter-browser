(function(window)
{
	var fields = %1;

	for (var i = 0; i < document.forms.length; ++i)
	{
		var isMatched = true;

		for (var j = 0; j < fields.length; ++j)
		{
			if (document.forms[i].querySelector('input[name=' + fields[j].name + ']' + ((fields[j].type == 'password') ? '[type=password]' : '')) == null)
			{
				isMatched = false;

				break;
			}
		}

		if (isMatched)
		{
			for (var j = 0; j < fields.length; ++j)
			{
				var element = document.forms[i].querySelector('input[name=' + fields[j].name + ']' + ((fields[j].type == 'password') ? '[type=password]' : ''));

				if (element !== null)
				{
					element.value = fields[j].value;
				}
			}
		}
	}
})(window);
