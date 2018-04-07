var title = '';
var isDefault = true;

for (var i = 0; i < document.styleSheets.length; ++i)
{
	if (document.styleSheets[i].ownerNode.rel.indexOf('alt') >= 0)
	{
		isDefault = false;

		break;
	}
}

if (!isDefault)
{
	var element = document.querySelector('link[rel=\'alternate stylesheet\']:not([disabled])');

	if (element)
	{
		title = element.title;
	}
}

title;
