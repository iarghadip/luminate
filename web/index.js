document.addEventListener("DOMContentLoaded", () => {
    let setMessageID = null;
    let nextScreen = false;
    const form = document.querySelector("form");
    const submit = form.querySelector("button[type=submit]");
    function setMessage(text, reset = true) {
        const e = document.getElementById("message");
        if (setMessageID !== null) {
            clearTimeout(setMessageID);
            setMessageID = null;
        }
        if (reset) {
            setMessageID = setTimeout(() => {
                e.textContent = nextScreen ? 'What are your preferences?' : 'Connect Luminate to Wi-Fi?';
                setMessageID = null;
            }, 2500);
        }
        e.textContent = text;
    }
    function parseJSON(response) {
        try {
            return JSON.parse(response);
        } catch {
            return JSON.parse(
                response
                    .replace(/'/g, '"')
                    .replace(/,\s*([}\]])/g, '$1')
                    .trim()
            );
        }
    }
    function isValidInputG1(wSsid, wPassword) {
        return (
            typeof wSsid === "string" &&
            typeof wPassword === "string" &&
            wSsid.length >= 1 &&
            wSsid.length <= 32 &&
            wPassword.length >= 8 &&
            wPassword.length <= 63
        );
    }
    function isValidInputG2(bCycle, bMinimum) {
        if (bCycle === "" || bMinimum === "") {
            return false;
        }
        const nCycle = Number(bCycle);
        const nMinimum = Number(bMinimum);
        return (
            !isNaN(nCycle) &&
            Number.isInteger(nCycle) &&
            nCycle >= 0 &&
            nCycle <= 23 &&
            !isNaN(nMinimum) &&
            nMinimum >= 0 &&
            nMinimum <= 50
        );
    }
    function createNestedSquares() {
        let current = document.createElement("div");
        current.className = "square black";
        document.querySelector(".container").appendChild(current);
        let isBlack = false;
        for (let i = 0; i < 30; i++) {
            const next = document.createElement("div");
            next.className = isBlack ? "square black" : "square";
            current.appendChild(next);
            current = next;
            isBlack = !isBlack;
        }
    }
    form.addEventListener("submit", e => {
        e.preventDefault();
        const wSsid = form.querySelector("input[name='{KEY_WIFI_SSID}']").value;
        const wPassword = form.querySelector("input[name='{KEY_WIFI_PASSWORD}']").value;
        const bCycle = form.querySelector("input[name='{KEY_BRIGHTNESS_CYCLE}']").value;
        const bMinimum = form.querySelector("input[name='{KEY_BRIGHTNESS_MINIMUM}']").value;
        if (!nextScreen) {
            if (isValidInputG1(wSsid, wPassword)) {
                submit.querySelector(".text").textContent = "Saving...";
                setTimeout(() => {
                    nextScreen = true;
                    document.getElementById("message").textContent = 'What are your preferences?';
                    document.getElementById("screen-1").style.display = "none";
                    document.getElementById("screen-2").style.display = "block";
                    submit.querySelector(".text").textContent = "Save Preferences";
                }, 1000);
            } else {
                setMessage("WiFi credentials are invalid!");
            }
            return;
        }
        if (!isValidInputG1(wSsid, wPassword)) {
            setMessage("WiFi credentials are invalid!");
            return;
        }
        if (!isValidInputG2(bCycle, bMinimum)) {
            setMessage("Brightness values are invalid!");
            return;
        }
        submit.querySelector(".text").textContent = "Saving...";
        form.querySelectorAll("input, button").forEach(e => e.disabled = true);
        setTimeout(() => {
            fetch("/wifi/save", {
                method: "POST",
                headers: {
                    "Content-Type": "application/x-www-form-urlencoded"
                },
                body: new URLSearchParams({
                    '{KEY_WIFI_SSID}': wSsid,
                    '{KEY_WIFI_PASSWORD}': wPassword,
                    '{KEY_BRIGHTNESS_CYCLE}': bCycle,
                    '{KEY_BRIGHTNESS_MINIMUM}': bMinimum
                })
            })
            .then(r => r.text())
            .then(response => {
                const json = parseJSON(response);
                if (json.success) {
                    setMessage(`Luminate is connected to ${wSsid}.`);
                    submit.querySelector(".text").textContent = "Completed";
                } else {
                    throw new Error(json.message || `Could not connect to ${wSsid}!`);
                }
                console.log("response:", json);
            })
            .catch(e => {
                setMessage(e.message || "Something went wrong!");
                submit.querySelector(".text").textContent = `Save ${nextScreen ? 'Preferences' : 'Network'}`;
                form.querySelectorAll("input, button").forEach(e => e.disabled = false);
            });
        }, 1000);
    });
    createNestedSquares();
    document.getElementById("screen-2").style.display = "none";
});