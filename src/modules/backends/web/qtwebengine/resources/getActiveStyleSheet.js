function getActiveStyleSheet()
{
	let title = '';
	let isDefault = true;

	for (let i = 0; i < document.styleSheets.length; ++i)
	{
		if (document.styleSheets[i].ownerNode.rel.indexOf('alt') >= 0)
		{
			isDefault = false;

			break;
		}
	}

	if (!isDefault)
	{
		let element = document.querySelector('link[rel=\'alternate stylesheet\']:not([disabled])');

		if (element)
		{
			title = element.title;
		}
	}

	return title;
}

getActiveStyleSheet();
