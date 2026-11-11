import os
import shutil
import zipfile
import subprocess
from fastapi import FastAPI, File, UploadFile, HTTPException
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

UPLOAD_DIR = "./temp_uploads"
EXTRACT_DIR = "./temp_extracted"

os.makedirs(UPLOAD_DIR, exist_ok=True)
os.makedirs(EXTRACT_DIR, exist_ok=True)

def remove_readonly(func, path, excinfo):
    os.chmod(path, 0o777)
    func(path)

@app.post("/api/upload")
async def upload_codebase(file: UploadFile = File(...)):
    if not file.filename.endswith(".zip"):
        raise HTTPException(status_code=400, detail="Only .zip files are allowed.")
    
    file_path = os.path.join(UPLOAD_DIR, file.filename)
    
    with open(file_path, "wb") as buffer:
        content = await file.read()
        buffer.write(content)
        
    folder_name = os.path.splitext(file.filename)[0]
    target_extract_path = os.path.join(EXTRACT_DIR, folder_name)
    
    if os.path.exists(target_extract_path):
        try:
            shutil.rmtree(target_extract_path, onerror=remove_readonly)
        except Exception:
            pass
    os.makedirs(target_extract_path, exist_ok=True)

    try:
        with zipfile.ZipFile(file_path, 'r') as zip_ref:
            zip_ref.extractall(target_extract_path)
    except zipfile.BadZipFile:
        raise HTTPException(status_code=400, detail="Invalid or corrupted zip file.")

    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    cpp_executable = os.path.join(base_dir, "scanner", "scanner")
    if not os.path.exists(cpp_executable) and os.path.exists(cpp_executable + ".exe"):
        cpp_executable = cpp_executable + ".exe"
    
    scan_results = "C++ engine not found or skipped"
    if os.path.exists(cpp_executable):
        try:
            result = subprocess.run(
                [cpp_executable, target_extract_path],
                capture_output=True,
                text=True,
                timeout=30
            )
            scan_results = result.stdout
        except Exception as e:
            scan_results = f"Error running C++ scanner: {str(e)}"
    else:
        scan_results = "Zip extracted successfully, but C++ binary path needs verification."

    return {
        "status": "success",
        "filename": file.filename,
        "extracted_path": target_extract_path,
        "scan_output": scan_results,
        "message": "Pipeline executed successfully!"
    }