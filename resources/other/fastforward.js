(function(isSelectingTheBestLink, hrefTokens, classTokens, idTokens, textTokens)
{
	const REL_SCORE = 9000;
	const TEXT_SCORE = 100;
	const ID_SCORE = 11;
	const CLASS_SCORE = 10;
	const HREF_SCORE = 1;
	const THRESHOLD = 10;

	function calculateScore(value, tokens, defaultScore)
	{
		var score = 0;

		if (value)
		{
			for (var i = (tokens.length - 1); i >= 0; --i)
			{
				if (value.indexOf(tokens[i].value) > -1)
				{
					score += (tokens[i].score || defaultScore);
				}
			}
		}

		return score;
	}

	function calculateScoreForValues(values, tokens, defaultScore)
	{
		var score = 0;

		if (values && values.length > 0)
		{
			for (var i = (values.length - 1); i >= 0; --i)
			{
				score += calculateScore(values[i].toUpperCase(), tokens, defaultScore);
			}
		}

		return score;
	}

	var links = document.querySelectorAll('a:not([href^="javascript:"]):not([href="#"])');
	var scoredLinks = [];

	for (var i = (links.length - 1); i >= 0; --i)
	{
		var score = 0;

		if (links[i].rel && links[i].rel.indexOf('next') > -1)
		{
			score += REL_SCORE;
		}

		score += calculateScore([links[i].innerText, links[i].getAttribute('aria-label'), links[i].getAttribute('alt'), links[i].title].join(' ').toUpperCase(), textTokens, TEXT_SCORE);
		score += calculateScore(links[i].id.toUpperCase(), idTokens, ID_SCORE);
		score += calculateScoreForValues(links[i].classList, classTokens, CLASS_SCORE);

		var url = links[i].pathname.split('/');

		if (links[i].search)
		{
			links[i].search.substr(1).split('&').forEach(function(item)
			{
				url.push(item.split('=')[0]);
			});
		}

		score += calculateScoreForValues(url, hrefTokens, HREF_SCORE);

		if (score > THRESHOLD)
		{
			if (isSelectingTheBestLink)
			{
				scoredLinks.push({score: score, link: links[i]});
			}
			else
			{
				return true;
			}
		}
	}

	if (scoredLinks.length > 0)
	{
		scoredLinks.sort(function(first, second)
		{
			return (second.score - first.score);
		});

		var link = document.createElement('a');
		link.href = scoredLinks[0].link.href;

		return link.href;
	}
	else
	{
		var url = window.location.href;
		if (!url.match(/(\d+)(\D*)$/))
		{
			return null;
		}
		var num = RegExp.$1;
		var digit = (num.charAt(0) == "0") ? num.length : null;
		num = parseInt(num) + 1;
		if (num < 0)
		{
			return null;
		}
		num = num.toString();
		// pad with zero
		digit = digit - num.length;
		for (var i = 0; i < digit; i++)
		{
			num = "0" + num;
		}
		return RegExp.leftContext + num + RegExp.$2;	
	}

	return null;
})({isSelectingTheBestLink}, {hrefTokens}, {classTokens}, {idTokens}, {textTokens})
