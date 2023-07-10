window.onload = function () {
    // Rom selection
    const roms = document.getElementById("roms");
    const load_rom = document.getElementById("load-rom");
    fetch(`http://${window.location.host}/api/get_roms`)
        .then(response => response.json())
        .then(data => {
            for (const rom of data.roms) {
                const option = document.createElement("option");
                option.value = rom;
                option.text = rom;
                roms.appendChild(option);
            }
            if (data.is_loaded) {
                roms.value = data.current_rom;
            }
        });
    load_rom.onclick = function (event) {
        event.preventDefault();
        const rom = roms.value;
        const body = JSON.stringify({rom: rom});
        fetch(`http://${window.location.host}/api/select_rom`, {
            method: "POST",
            body: body
        });
    }

    // Patch selection
    const patches = document.getElementById("patches");
    const load_patch = document.getElementById("load-patch");
    fetch(`http://${window.location.host}/api/get_patches`)
        .then(response => response.json())
        .then(data => {
            for (const patch of data.patches) {
                const option = document.createElement("option");
                option.value = patch;
                option.text = patch;
                patches.appendChild(option);
            }
            if (data.is_loaded) {
                patches.value = data.current_patch;
            }
        });
    load_patch.onclick = function (event) {
        event.preventDefault();
        const patch = patches.value;
        const body = JSON.stringify({patch: patch});
        fetch(`http://${window.location.host}/api/select_patch`, {
            method: "POST",
            body: body
        }).then(loadParams);
    }

    // Voice params
    loadParams = function () {
        const voice_params = document.getElementById("voice-params");
        fetch(`http://${window.location.host}/api/get_params`)
            .then(response => response.json())
            .then(data => {
                // voice_params.innerHTML = JSON.stringify(data.params, null, 2);
            });
    };
    loadParams();

    // Visualization
    fetch(`http://${window.location.host}/api/viz_stream`)
        .then(async (response) => {
            const reader = response.body.getReader();
            for await (const chunk of readChunks(reader)) {
                if (chunk.buffer.byteLength % 4 !== 0) {
                    console.log("Invalid chunk size");
                    continue;
                }
                const samples = new Int32Array(chunk.buffer);
                let max = 0;
                for (let i = 0; i < samples.length; i++) {
                    const sample = samples[i] / 32768;
                    samples[i] = sample;
                    if (Math.abs(sample) > max) {
                        max = Math.abs(sample);
                    }
                }
                const canvas = document.getElementById("viz");
                const ctx = canvas.getContext("2d");
                const width = canvas.width;
                const height = canvas.height;
                const xStep = width / samples.length;
                const yStep = height / 2 / max;
                ctx.strokeStyle = "#ffffff";
                ctx.lineWidth = 2;
                ctx.clearRect(0, 0, width, height);
                ctx.beginPath();
                for (let i = 0; i < samples.length; i++) {
                    if (i === 0) {
                        ctx.moveTo(0, height / 2 + samples[i] * yStep);
                    }
                    ctx.lineTo(i * xStep, height / 2 + samples[i] * yStep);
                }
                ctx.stroke();
            }
        })

    function readChunks(reader) {
        return {
            async* [Symbol.asyncIterator]() {
                let readResult = await reader.read();
                while (!readResult.done) {
                    yield readResult.value;
                    readResult = await reader.read();
                }
            },
        };
    }

    // Play MIDI with keyboard
    qwertyNotes = [];
    qwertyNotes[16] = 41;
    // = F2
    qwertyNotes[65] = 42;
    qwertyNotes[90] = 43;
    qwertyNotes[83] = 44;
    qwertyNotes[88] = 45;
    qwertyNotes[68] = 46;
    qwertyNotes[67] = 47;
    qwertyNotes[86] = 48;
    // = C3
    qwertyNotes[71] = 49;
    qwertyNotes[66] = 50;
    qwertyNotes[72] = 51;
    qwertyNotes[78] = 52;
    qwertyNotes[77] = 53;
    // = F3
    qwertyNotes[75] = 54;
    qwertyNotes[188] = 55;
    qwertyNotes[76] = 56;
    qwertyNotes[190] = 57;
    qwertyNotes[186] = 58;
    qwertyNotes[191] = 59;

    // Upper row: q2w3er5t6y7u...
    qwertyNotes[81] = 60;
    // = C4 ("middle C")
    qwertyNotes[50] = 61;
    qwertyNotes[87] = 62;
    qwertyNotes[51] = 63;
    qwertyNotes[69] = 64;
    qwertyNotes[82] = 65;
    // = F4
    qwertyNotes[53] = 66;
    qwertyNotes[84] = 67;
    qwertyNotes[54] = 68;
    qwertyNotes[89] = 69;
    qwertyNotes[55] = 70;
    qwertyNotes[85] = 71;
    qwertyNotes[73] = 72;
    // = C5
    qwertyNotes[57] = 73;
    qwertyNotes[79] = 74;
    qwertyNotes[48] = 75;
    qwertyNotes[80] = 76;
    qwertyNotes[219] = 77;
    // = F5
    qwertyNotes[187] = 78;
    qwertyNotes[221] = 79;

    const ws_midi = new WebSocket(`ws://${window.location.host}/api/midi`);
    document.addEventListener("keydown", function (event) {
        if (event.repeat) return;
        const note = qwertyNotes[event.keyCode];
        console.log(event.keyCode, note);
        if (note !== undefined) {
            ws_midi.send(new Uint8Array([0x90, note, 0x40]));
        }
    });
    document.addEventListener("keyup", function (event) {
        if (event.repeat) return;
        const note = qwertyNotes[event.keyCode];
        if (note !== undefined) {
            ws_midi.send(new Uint8Array([0x80, note, 0x40]));
        }
    });

    const noteToIndex = {
        c: 0,
        cs: 1,
        d: 2,
        ds: 3,
        e: 4,
        f: 5,
        fs: 6,
        g: 7,
        gs: 8,
        a: 9,
        as: 10,
        b: 11
    }

    // Keyboard mouse events
    const keyboard_element = document.querySelectorAll(".keyboard-keys>.key");
    function pressKey(key) {
        if (!key.classList.contains("active")) {
            key.classList.add("active");
            const note = key.getAttribute("data-note");
            const octave = key.getAttribute("data-octave");
            const midiNote = noteToIndex[note] + parseInt(octave) * 12 + 36;
            console.log("Press", note, octave, midiNote);
            ws_midi.send(new Uint8Array([0x90, midiNote, 0x40]));
        }
    }
    function releaseKey(key) {
        if (key.classList.contains("active")) {
            key.classList.remove("active");
            const note = key.getAttribute("data-note");
            const octave = key.getAttribute("data-octave");
            const midiNote = noteToIndex[note] + parseInt(octave) * 12 + 36;
            console.log("Release", note, octave, midiNote);
            ws_midi.send(new Uint8Array([0x80, midiNote, 0x40]));
        }
    }
    keyboard_element.forEach(key => {
        key.addEventListener("mousedown", function (event) {
            pressKey(key);
        });
        key.addEventListener("mouseup", function (event) {
            releaseKey(key);
        });
        key.addEventListener("mouseleave", function (event) {
            releaseKey(key);
        });
        key.addEventListener("mouseover", function (event) {
            if (event.buttons === 1) {
                pressKey(key);
            }
        });
    });

    // Init
    fetch("/api/init", {method: "POST"});
}
