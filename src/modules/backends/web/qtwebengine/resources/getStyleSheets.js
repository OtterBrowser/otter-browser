let elements = document.querySelectorAll('link[rel=\'alternate stylesheet\']');
let titles = [];

for (let i = 0; i < elements.length; ++i)
{
	if (elements[i].title !== '' && !titles.includes(elements[i].title))
	{
		titles.push(elements[i].title);
	}

}

titles;
