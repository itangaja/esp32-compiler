const express = require("express");
const cors = require("cors");
const bodyParser = require("body-parser");
const { exec } = require("child_process");
const fs = require("fs-extra");
const path = require("path");
const util = require("util");

const execPromise = util.promisify(exec);
const app = express();
const PORT = 5001;

app.use(cors());
app.use(bodyParser.json());

// Pastikan backend bisa mengakses frontend yang ada di luar folder backend
app.use(express.static(path.join(__dirname, "../frontend")));

app.get("/", (req, res) => {
    res.sendFile(path.join(__dirname, "../frontend", "index.html"));
});

const projectsDir = path.join(__dirname, "projects");
fs.ensureDirSync(projectsDir);

app.post("/generate", async (req, res) => {
    const { code } = req.body;

    if (!code) {
        return res.status(400).json({ error: "No code provided" });
    }

    const projectPath = path.join(projectsDir, "esp_project");
    const srcPath = path.join(projectPath, "src");
    const mainFilePath = path.join(srcPath, "main.cpp");
    const iniFilePath = path.join(projectPath, "platformio.ini");

    try {
        await fs.ensureDir(srcPath);
        await fs.writeFile(mainFilePath, code);

        // Buat atau update platformio.ini
        const iniContent = `[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    me-no-dev/AsyncTCP
    me-no-dev/ESPAsyncWebServer
    adafruit/Adafruit Unified Sensor
    adafruit/DHT sensor library
`;
        await fs.writeFile(iniFilePath, iniContent);

        console.log("Inisialisasi proyek...");
        await execPromise("pio project init --board esp32dev", { cwd: projectPath });

        console.log("Menginstal library...");
        await execPromise("pio lib install", { cwd: projectPath });

        console.log("Mengompilasi kode...");
        const { stdout, stderr } = await execPromise("pio run", { cwd: projectPath });

        if (stderr) {
            console.error("Compilation stderr:", stderr);
        }
        console.log(stdout);

        const firmwarePath = path.join(projectPath, ".pio/build/esp32dev/firmware.bin");

        if (fs.existsSync(firmwarePath)) {
            return res.download(firmwarePath, "firmware.bin");
        } else {
            return res.status(500).json({ error: "Firmware file not found" });
        }
    } catch (err) {
        console.error("Error:", err);
        return res.status(500).json({ error: "Internal server error", details: err.message });
    }
});

app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});
