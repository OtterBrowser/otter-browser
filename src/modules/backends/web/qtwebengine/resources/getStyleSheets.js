var elements = document.querySelectorAll('link[rel=\'alternate stylesheet\']');
var titles = [];

for (var i = 0; i < elements.length; ++i)
{
	if (elements[i].title !== '' && !titles.includes(elements[i].title))
	{
		titles.push(elements[i].title);
	}

}

titles;
