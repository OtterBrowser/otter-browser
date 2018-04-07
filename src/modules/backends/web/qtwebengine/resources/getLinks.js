var elements = document.querySelectorAll('%1');
var urls = [];
var links = [];

for (var i = 0; i < elements.length; ++i)
{
	if (urls.includes(elements[i].href))
	{
		continue;
	}

	urls.push(elements[i].href);

	var link = {
		title: elements[i].title.trim(),
		mimeType: elements[i].type,
		url: elements[i].href
	};

	if (link.title == '')
	{
		link.title = elements[i].textContent.trim();
	}

	if (link.title == '')
	{
		var imageElement = elements[i].querySelector('img[alt]:not([alt=\'\'])');

		if (imageElement)
		{
			link.title = imageElement.alt;
		}
	}

	links.push(link);
}

links;
