#define clientPage "" \
    "<!DOCTYPE html>\n" \
    "<html lang='en'>\n" \
    "\n" \
    "    <head>\n" \
    "        <title>Good-looking controls</title>\n" \
    "\n" \
    "        <meta charset='UTF-8'>\n" \
    "        <meta http-equiv='X-UA-Compatible' content='IE=edge'>\n" \
    "        <meta name='viewport' content='width=device-width,initial-scale=1.0'>\n" \
    "\n" \
    "        <style>\n" \
    "\n" \
    "            hr {\n" \
    "                border: 0;\n" \
    "                border-top: 1px solid lightgray;\n" \
    "                border-bottom: 1px solid lightgray;\n" \
    "            }\n" \
    "\n" \
    "            h1 {\n" \
    "                font-family: verdana;\n" \
    "                font-size: 40px; text-align: center;\n" \
    "            }\n" \
    "\n" \
    "            div.d1 {position: relative;\n" \
    "                width: 100%;\n" \
    "                height: 62px;\n" \
    "            }\n" \
    "            div.d2 {\n" \
    "                position: relative;\n" \
    "                float: left;\n" \
    "                width: 42%;\n" \
    "                font-family: verdana;\n" \
    "                font-size: 32px;\n" \
    "                color: gray;\n" \
    "            }\n" \
    "            div.d3 {\n" \
    "                position: relative;\n" \
    "                float: left;\n" \
    "                width: 58%;\n" \
    "                font-family: verdana;\n" \
    "                font-size: 32px;\n" \
    "                color: black;\n" \
    "            }\n" \
    "\n" \
    "            /* switch control */\n" \
    "            .switch {\n" \
    "                position: relative;\n" \
    "                display: inline-block;\n" \
    "                width: 86px;\n" \
    "                height: 46px;\n" \
    "            }\n" \
    "            .slider {\n" \
    "                position: absolute;\n" \
    "                cursor: pointer;\n" \
    "                top: 0;\n" \
    "                left: 0;\n" \
    "                right: 0;\n" \
    "                bottom: 0;\n" \
    "                background-color: #ccc;\n" \
    "                -webkit-transition: .4s;\n" \
    "                transition: .4s;\n" \
    "            }\n" \
    "            .slider:before {\n" \
    "                position: absolute;\n" \
    "                content: '';\n" \
    "                height: 38px;\n" \
    "                width: 38px;\n" \
    "                left: 4px;\n" \
    "                bottom: 4px;\n" \
    "                background-color: white;\n" \
    "                -webkit-transition: .4s;\n" \
    "                transition: .4s;\n" \
    "            }\n" \
    "            input:checked+.slider {\n" \
    "                background-color: hsl(207,90%,30%);\n" \
    "            }\n" \
    "            input:focus+.slider {\n" \
    "                box-shadow: 0 0 1px hsl(207,90%,30%);\n" \
    "            }\n" \
    "            input:checked+.slider:before {\n" \
    "                -webkit-transform: translateX(40px);\n" \
    "                -ms-transform: translateX(40px);\n" \
    "                transform: translateX(40px);\n" \
    "            }\n" \
    "            .switch input {\n" \
    "                display: none\n" \
    "            }\n" \
    "            .slider.round {\n" \
    "                border-radius: 30px;\n" \
    "            }\n" \
    "            .slider.round:before {\n" \
    "                border-radius: 50%;\n" \
    "            }\n" \
    "            input:disabled+.slider {\n" \
    "                background-color: #aaa;\n" \
    "            }\n" \
    "\n" \
    "            /* slider control */\n" \
    "            input[type='range'] {\n" \
    "                -webkit-appearance: none;\n" \
    "                -webkit-tap-highlight-color: rgba(255,255,255,0);\n" \
    "                width: 94%;\n" \
    "                height: 36px;\n" \
    "                margin: 0;\n" \
    "                border: none;\n" \
    "                padding: 4px 4px;\n" \
    "                border-radius: 28px;\n" \
    "                background: hsl(207,90%,30%);\n" \
    "                outline: none;\n" \
    "            }\n" \
    "            input[type='range']::-moz-range-track {\n" \
    "                border: inherit;\n" \
    "                background: transparent;\n" \
    "            }\n" \
    "            input[type='range']::-ms-track {\n" \
    "                border: inherit;\n" \
    "                color: transparent;\n" \
    "                background: transparent;\n" \
    "            }\n" \
    "            input[type='range']::-ms-fill-lower\n" \
    "            input[type='range']::-ms-fill-upper {\n" \
    "                background: transparent;\n" \
    "            }\n" \
    "            input[type='range']::-ms-tooltip {\n" \
    "                display: none;\n" \
    "            }\n" \
    "            input[type='range']::-webkit-slider-thumb {\n" \
    "                -webkit-appearance: none;\n" \
    "                width: 46px;\n" \
    "                height: 36px;\n" \
    "                border: none;\n" \
    "                border-radius: 20px;\n" \
    "                background-color: white;\n" \
    "            }\n" \
    "            input[type='range']::-moz-range-thumb {\n" \
    "                width: 46px;\n" \
    "                height: 36px;\n" \
    "                border: none;\n" \
    "                border-radius: 20px;\n" \
    "                background-color: white;\n" \
    "            }\n" \
    "            input[type='range']::-ms-thumb {\n" \
    "                width: 46px;\n" \
    "                height: 36px;\n" \
    "                border-radius: 20px;\n" \
    "                border: 0;\n" \
    "                background-color: white;\n" \
    "            }\n" \
    "            input:disabled+.slider {\n" \
    "                background-color: #aaa;\n" \
    "            }\n" \
    "            input[type='range']:disabled {\n" \
    "                background: #aaa;\n" \
    "            }\n" \
    "\n" \
    "            /* radio button control */\n" \
    "            .container {\n" \
    "                display: black;\n" \
    "                position: relative;\n" \
    "                padding-left: 222px;\n" \
    "                margin-bottom: 14px;\n" \
    "                cursor: pointer;\n" \
    "                font-family: verdana;\n" \
    "                font-size: 30px;\n" \
    "                color: gray;\n" \
    "                -webkit-user-select: none;\n" \
    "                -moz-user-select: none;\n" \
    "                -ms-user-select: none;\n" \
    "                user-select: none;\n" \
    "            }\n" \
    "            .container input {\n" \
    "                position: absolute;\n" \
    "                opacity: 0;\n" \
    "                cursor: pointer;\n" \
    "                height: 0;\n" \
    "                width: 0;\n" \
    "            }\n" \
    "            .checkmark {\n" \
    "                position: absolute;\n" \
    "                top: 0;\n" \
    "                left: 0;\n" \
    "                height: 46px;\n" \
    "                width: 46px;\n" \
    "                background-color: #ddd;\n" \
    "                border-radius: 50%;\n" \
    "            }\n" \
    "            .container:hover input ~ .checkmark {\n" \
    "                background-color: #ccc;\n" \
    "            }\n" \
    "            .container input:checked ~ .checkmark {\n" \
    "                background-color: hsl(207,90%,20%);\n" \
    "            }\n" \
    "            .checkmark:after {\n" \
    "                content: '';\n" \
    "                position: absolute;\n" \
    "                display: none;\n" \
    "            }\n" \
    "            .container input:checked ~ .checkmark:after {\n" \
    "                display: block;\n" \
    "            }\n" \
    "            .container .checkmark:after {\n" \
    "                top: 11px;\n" \
    "                left: 11px;\n" \
    "                width: 24px;\n" \
    "                height: 24px;\n" \
    "                border-radius: 50%;\n" \
    "                background: white;\n" \
    "            }\n" \
    "            .container input:disabled ~ .checkmark {\n" \
    "                background-color: gray;\n" \
    "            }\n" \
    "            .container input:disabled ~ .checkmark {\n" \
    "                background-color: #aaa;\n" \
    "            }\n" \
    "\n" \
    "            /* button control */\n" \
    "            .button {\n" \
    "                padding: 10px 15px;\n" \
    "                font-size: 26px;\n" \
    "                text-align: center;\n" \
    "                cursor: pointer;\n" \
    "                outline: none;\n" \
    "                color: white;\n" \
    "                border: none;\n" \
    "                border-radius: 12px;\n" \
    "                box-shadow: 1px 1px #ccc;\n" \
    "                position: relative;\n" \
    "                top: 0px;\n" \
    "                height: 48px;\n" \
    "            }\n" \
    "            button:disabled {\n" \
    "                background-color: #aaa;\n" \
    "            }\n" \
    "            button:disabled:hover {\n" \
    "                background-color: #aaa;\n" \
    "            }\n" \
    "            /* blue button */\n" \
    "            .button1 {\n" \
    "                background-color: hsl(207,90%,40%);\n" \
    "             }\n" \
    "            .button1:hover {\n" \
    "                background-color: hsl(207,90%,30%);\n" \
    "             }\n" \
    "            .button1:active {\n" \
    "                background-color: hsl(207,90%,30%);\n" \
    "                transform: translateY(3px);\n" \
    "            }\n" \
    "            /* green button */\n" \
    "            .button2 {\n" \
    "                background-color: hsl(82,90%,25%);\n" \
    "             }\n" \
    "            .button2:hover {\n" \
    "                background-color: hsl(82,90%,20%);\n" \
    "             }\n" \
    "            .button2:active {\n" \
    "                background-color: hsl(82,90%,20%);\n" \
    "                transform: translateY(3px);\n" \
    "            }\n" \
    "            /* red button */\n" \
    "            .button3 {\n" \
    "                background-color: hsl(0,100%,25%);\n" \
    "             }\n" \
    "            .button3:hover {\n" \
    "                background-color: hsl(0,100%,20%);\n" \
    "             }\n" \
    "            .button3:active {\n" \
    "                background-color: hsl(0,100%,20%);\n" \
    "                transform: translateY(3px);\n" \
    "            }\n" \
    "\n" \
    "        </style>\n" \
    "    </head>\n" \
    "\n" \
    "    <body>\n" \
    "\n" \
    "        <br><h1>Good-looking controls</h1>\n" \
    "        <hr />\n" \
    "\n" \
    "        <div class='d1'>\n" \
    "            <div class='d2'>&nbsp;Switch</div>\n" \
    "            <div class='d3'>\n" \
    "                <label class='switch'><input type='checkbox' id='mySwitch' disabled onClick='syncSwitchWithEsp (id, checked)'><div class='slider round'></div></label>\n" \
    "            </div>\n" \
    "        </div>\n" \
    "\n" \
    "        <hr />\n" \
    "        <div class='d1'>\n" \
    "            <div class='d2'>&nbsp;Slider</div>\n" \
    "            <div class='d3'>\n" \
    "                <input id='mySlider' type='range' min='0' max='10' value='0' step='1' disabled style='width: 60%;' onchange=\"\n" \
    "\n" \
    "                    // do not react on every change since there may be too many of them - react only every 1/3 of second\n" \
    "                    clearTimeout (sliderTimeout);\n" \
    "                    sliderTimeout = setTimeout (function () {\n" \
    "                        client.request ('/mySlider/' + document.getElementById ('mySlider').value,'PUT',function (json) { alert ('Reply from HTTP server: ' + json); });\n" \
    "                    },333);\n" \
    "\n" \
    "                \"/>\n" \
    "            </div>\n" \
    "        </div>\n" \
    "\n" \
    "        <hr />\n" \
    "        <div class='d1'>\n" \
    "            <div class='d2'>&nbsp;Radio button 1</div>\n" \
    "            <div class='d3'>\n" \
    "                <label class='container'>&nbsp;<input type='radio' checked='checked' name='myRadioButtons' id='option1' disabled onchange=\"\n" \
    "\n" \
    "                    client.request ('/' + this.name + '/' + id,'PUT', function (json) {\n" \
    "                        if (JSON.parse (json).value == 'option1') document.getElementById ('option1').checked = 'checked';\n" \
    "                        else                                      document.getElementById ('option2').checked = 'checked';\n" \
    "                        alert ('Reply from HTTP server: ' + json);\n" \
    "                    });\n" \
    "\n" \
    "                \"><span class='checkmark'></span></label>\n" \
    "            </div>\n" \
    "        </div>\n" \
    "        <div class='d1'>\n" \
    "            <div class='d2'>&nbsp;Radio button 2</div>\n" \
    "            <div class='d3'>\n" \
    "                <label class='container'>&nbsp;<input type='radio' name='myRadioButtons' id='option2' disabled onchange=\"\n" \
    "\n" \
    "                    client.request ('/' + name + '/' + id, 'PUT', function (json) {\n" \
    "                        if (JSON.parse (json).value == 'option1') document.getElementById ('option1').checked = 'checked';\n" \
    "                        else                                      document.getElementById ('option2').checked = 'checked';\n" \
    "                        alert ('Reply from HTTP server: ' + json);\n" \
    "                    });\n" \
    "\n" \
    "                \"><span class='checkmark'></span></label>\n" \
    "            </div>\n" \
    "        </div>\n" \
    "\n" \
    "        <hr />\n" \
    "        <div class='d1'; style='height: 46px;'>\n" \
    "            <div class='d2'>&nbsp;Buttons</div>\n" \
    "\n" \
    "            <div class='d3'>\n" \
    "                <button class='button button1' id='blueButton' onclick=\"\n" \
    "\n" \
    "                    client.request ('/' + id + '/pressed', 'PUT', function (json) {\n" \
    "                        alert ('Reply from HTTP server: ' + json);\n" \
    "                    });\n" \
    "\n" \
    "                \">&nbsp;PRESS&nbsp;</button>\n" \
    "                <button class='button button2' id='greenButton' onclick=\"\n" \
    "\n" \
    "                    client.request ('/' + id + '/pressed', 'PUT', function (json) {\n" \
    "                        alert ('Reply from HTTP server: ' + json);\n" \
    "                    });\n" \
    "\n" \
    "                \">&nbsp;PRESS&nbsp;</button>\n" \
    "\n" \
    "                <button class='button button3' id='redButton' onclick=\"\n" \
    "\n" \
    "                    client.request ('/' + id + '/pressed', 'PUT', function (json) {\n" \
    "                        alert ('Reply from HTTP server: ' + json);\n" \
    "                    });\n" \
    "\n" \
    "                \">&nbsp;PRESS&nbsp;</button>\n" \
    "            </div>\n" \
    "        </div>\n" \
    "\n" \
    "        <hr />\n" \
    "    </body>\n" \
    "\n" \
    "    <script type='text/javascript'>\n" \
    "\n" \
    "        // mechanism that makes REST calls and get their replies\n" \
    "        var httpClient = function () {\n" \
    "            this.request = function (url,method,callback) {\n" \
    "                var httpRequest = new XMLHttpRequest ();\n" \
    "                var httpRequestTimeout = null;\n" \
    "\n" \
    "                httpRequest.onreadystatechange = function () {\n" \
    "                    if (httpRequest.readyState == 1) { // 1 = OPENED, start timing\n" \
    "                        clearTimeout (httpRequestTimeout);\n" \
    "                        httpRequestTimeout = setTimeout (function () {\n" \
    "                            alert ('HTTP server did not reply (in time).');\n" \
    "                        },5000);\n" \
    "                    }\n" \
    "                    if (httpRequest.readyState == 4) { // 4 = DONE, call callback function with responseText\n" \
    "                        clearTimeout (httpRequestTimeout);\n" \
    "                        switch (httpRequest.status) {\n" \
    "                            case 200:     callback (httpRequest.responseText); // 200 = OK\n" \
    "                                        break;\n" \
    "                            case 0:        break;\n" \
    "                            default:     alert ('HTTP server reported an error ' + httpRequest.status + ' ' + httpRequest.responseText); // some other reply status, like 404, 503,...\n" \
    "                                        break;\n" \
    "                        }\n" \
    "                    }\n" \
    "                }\n" \
    "                httpRequest.open (method, url, true);\n" \
    "                httpRequest.send (null);\n" \
    "            }\n" \
    "        }\n" \
    "\n" \
    "        var client = new httpClient ();\n" \
    "\n" \
    "        var sliderTimeout = null;\n" \
    "\n" \
    "        // send switch state to ESP and refresh switch with ESP response\n" \
    "        function syncSwitchWithEsp (switchId, switchState) {\n" \
    "            client.request ('/' + switchId + '/' + switchState, 'PUT', function (json) {\n" \
    "                console.log (json);\n" \
    "                var obj = document.getElementById (switchId);\n" \
    "                obj.checked = (JSON.parse (json).value == 'true');\n" \
    "                alert ('Reply from HTTP server: ' + json);\n" \
    "            });\n" \
    "        }\n" \
    "\n" \
    "        // initialize/populate this page controls at page load\n" \
    "        client.request ('/mySwitch', 'GET', function (json) {\n" \
    "            console.log (json);\n" \
    "            var obj = document.getElementById ('mySwitch');\n" \
    "            obj.disabled = false;\n" \
    "            obj.checked = (JSON.parse (json).value == 'true');\n" \
    "        });\n" \
    "\n" \
    "        client.request ('/mySlider', 'GET', function (json) {\n" \
    "            console.log (json);\n" \
    "            var obj = document.getElementById ('mySlider');\n" \
    "            obj.disabled = false;\n" \
    "            obj.value = JSON.parse (json).value;\n" \
    "        });\n" \
    "\n" \
    "        client.request ('/myRadioButtons', 'GET', function (json) {\n" \
    "            console.log (json);\n" \
    "            document.getElementById ('option1').disabled = false;\n" \
    "            document.getElementById ('option2').disabled = false;\n" \
    "            if (JSON.parse (json).value == 'option1') document.getElementById ('option1').checked = 'checked';\n" \
    "            else                                      document.getElementById ('option2').checked = 'checked';\n" \
    "        });\n" \
    "\n" \
    "    </script>\n" \
    "\n" \
    "</html>\n"

