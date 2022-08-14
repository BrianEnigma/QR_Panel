function itohex(value)
{
    var result = Number(value).toString(16);
    return result.length == 1 ? "0" + result : result;
}

function buildColor(triplet)
{
    let result = '#';
    for (let i = 0; i < 3; i++)
        result += itohex(triplet[i]);
    return result;
}

function parseColors(data, textStatus)
{
    console.log('Ajax Success');
    const colorName1 = data.color_names[0];
    const colorName2 = data.color_names[1];
    const color1 = data.colors[0];
    const color2 = data.colors[1];
    console.log('' + color1 + ' ' + colorName1 + ' ' + color2 + ' ' + colorName2);
    $('#leftColorSwatch').css('background-color', buildColor(color1));
    $('#rightColorSwatch').css('background-color', buildColor(color2));
    $('#leftColorName').html(colorName1);
    $('#rightColorName').html(colorName2);
    $('#result').show();
    $('#wait').hide();
}

function displayError(jqXHR, textStatus, errorThrown)
{
    console.log('Error: ' + errorThrown + ' / ' + textStatus);
    $('#wait').html(errorThrown);
}
function documentLoaded()
{
    $('#result').hide();
    $.ajax({
        url: 'https://XXXXXXXXXX.execute-api.us-west-2.amazonaws.com/prod',
        type: 'GET',
        dataType: 'json'
    })
        .done(parseColors)
        .fail(displayError);
}

