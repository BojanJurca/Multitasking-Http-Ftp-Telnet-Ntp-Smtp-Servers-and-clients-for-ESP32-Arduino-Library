// this file has been automatically generated from administration.html with html2h.vbs

#define administration "" \
    "" \
    "<!DOCTYPE html>\n" \
    "<html lang='en'>\n" \
    "    <head>\n" \
    "        <title>ESP32 administration</title>\n" \
    "\n" \
    "        <meta charset='UTF-8'>\n" \
    "        <meta http-equiv='X-UA-Compatible' content='IE=edge'>\n" \
    "        <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n" \
    "\n" \
    "        <!-- don't cache - at least during development -->\n" \
    "        <meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate, max-age=0'>\n" \
    "        <meta http-equiv='Pragma' content='no-cache'>\n" \
    "        <meta http-equiv='Expires' content='0'>\n" \
    "\n" \
    "\n" \
    "\n" \
    "\n" \
    "        <style>\n" \
    "\n" \
    "            h1 {font-family: verdana; font-size: 32px; text-align: center}\n" \
    "\n" \
    "            div.d1 {\n" \
    "                position: relative;\n" \
    "                height: 90px;\n" \
    "                width: 100%;\n" \
    "                font-family: verdana;\n" \
    "                font-size: 30px;\n" \
    "                color: hsl(207, 90%, 30%);\n" \
    "            }\n" \
    "\n" \
    "            .button {\n" \
    "                padding: 10px 20px;\n" \
    "                font-size: 18px;\n" \
    "                border-radius: 10px;\n" \
    "                border: none;\n" \
    "                color: white;\n" \
    "                cursor: pointer;\n" \
    "            }\n" \
    "            .button:active {\n" \
    "                transform: translateY(3px)\n" \
    "            }\n" \
    "            .button:disabled {\n" \
    "                background-color: #aaa !important;\n" \
    "            }\n" \
    "            .button1 {\n" \
    "                background-color: hsl(207, 90%, 40%)\n" \
    "            }\n" \
    "            .button1:hover {\n" \
    "                background-color: hsl(207, 90%, 30%)\n" \
    "            }\n" \
    "            .button1:active {\n" \
    "                background-color: hsl(207, 90%, 30%);\n" \
    "            }\n" \
    "\n" \
    "            dialog[open] {\n" \
    "                font-family: Verdana, sans-serif;\n" \
    "                font-size: 32px;\n" \
    "                color: white;\n" \
    "                background-color: rgba(200, 0, 0, 0.92);\n" \
    "                border: none;\n" \
    "                border-radius: 12px;\n" \
    "                padding: 18px 26px;\n" \
    "                box-shadow: 0 4px 18px rgba(0, 0, 0, 0.35);\n" \
    "                position: fixed;\n" \
    "                top: 50%;\n" \
    "                left: 50%;\n" \
    "                transform: translate(-50%, -50%);\n" \
    "                animation: fadeInScale 0.18s ease-out;\n" \
    "            }\n" \
    "            @keyframes fadeInScale {\n" \
    "                from {\n" \
    "                    opacity: 0;\n" \
    "                    transform: translate(-50%, -50%) scale(0.92);\n" \
    "                }\n" \
    "                to {\n" \
    "                    opacity: 1;\n" \
    "                    transform: translate(-50%, -50%) scale(1);\n" \
    "                }\n" \
    "            }\n" \
    "\n" \
    "            body {\n" \
    "                font-family: Verdana, sans-serif;\n" \
    "                margin: 10px;\n" \
    "                overflow-wrap: break-word;\n" \
    "            }\n" \
    "\n" \
    "        </style>\n" \
    "    </head>\n" \
    "\n" \
    "    <body>\n" \
    "        \n" \
    "        <br><h1>ESP32 administration</h1>\n" \
    "        <br>\n" \
    "        <div class='d1'>\n" \
    "            <b>Place your own content here</b>\n" \
    "        </div>\n" \
    "\n" \
    "        <div style='width:100%; text-align:right; padding-right:20px; margin-top:20px;'>\n" \
    "            <button class='button button1' onclick='logout()'>Logout</button>\n" \
    "        </div>\n" \
    "\n" \
    "        <script type='text/javascript'>\n" \
    "\n" \
    "            async function logout() {\n" \
    "                const reply = await httpRequest('/logout', 'POST');\n" \
    "                if (reply == 'OK')\n" \
    "                    window.location.href = '/index.html';\n" \
    "                else\n" \
    "                    alert(reply);\n" \
    "            }\n" \
    "\n" \
    "            // -------------------------------------------------------------\n" \
    "            // ERROR MESSAGE\n" \
    "            // -------------------------------------------------------------\n" \
    "            let errorMessageTimeout = null;\n" \
    "\n" \
    "            function errorMessage(msg) {\n" \
    "                clearTimeout(errorMessageTimeout);\n" \
    "                errDialog.textContent = msg;\n" \
    "                errDialog.showModal();\n" \
    "                errorMessageTimeout = setTimeout(() => errDialog.close(), 3000);\n" \
    "            }\n" \
    "\n" \
    "\n" \
    "            // -------------------------------------------------------------\n" \
    "            // GLOBAL FETCH CLIENT WITH TIMEOUT\n" \
    "            // -------------------------------------------------------------\n" \
    "            async function httpRequest(url, method = 'GET') {\n" \
    "                const controller = new AbortController();\n" \
    "                const t = setTimeout(() => controller.abort(), 5000);\n" \
    "\n" \
    "                try {\n" \
    "                    const response = await fetch(url, {\n" \
    "                        method,\n" \
    "                        signal: controller.signal\n" \
    "                    });\n" \
    "\n" \
    "                    clearTimeout(t);\n" \
    "\n" \
    "                    if (!response.ok)\n" \
    "                        throw new Error('Server reported error ' + response.status);\n" \
    "\n" \
    "                    return await response.text();\n" \
    "\n" \
    "                } catch (err) {\n" \
    "                    if (err.name === 'AbortError')\n" \
    "                        errorMessage('Server did not reply in time.');\n" \
    "                    else\n" \
    "                        errorMessage(err.message);\n" \
    "\n" \
    "                    throw err;\n" \
    "                }\n" \
    "            }\n" \
    "\n" \
    "        </script>\n" \
    "    </body>\n" \
    "</html>\n" \
    ""
