document.getElementById('uploadForm').addEventListener('submit', async (e) => {
    e.preventDefault();

    const fileInput = document.getElementById('zipFile');
    const responseDiv = document.getElementById('response');

    if (fileInput.files.length === 0) {
        responseDiv.innerText = "Please select a .zip file.";
        return;
    }

    const formData = new FormData();
    formData.append("file", fileInput.files[0]);

    responseDiv.innerText = "Uploading...";

    try {
        const res = await fetch('http://localhost:8000/api/upload', {
            method: 'POST',
            body: formData
        });

        const data = await res.json();

        if (res.ok) {
            responseDiv.style.color = "green";
            responseDiv.innerText = `Success: ${data.message} (${data.filename})\n\nScan Results:\n${data.scan_output}`;
        } else {
            responseDiv.style.color = "red";
            responseDiv.innerText = `Error: ${data.detail || "Upload failed"}`;
        }
    } catch (err) {
        responseDiv.style.color = "red";
        responseDiv.innerText = "Network error: Could not reach backend.";
    }
});