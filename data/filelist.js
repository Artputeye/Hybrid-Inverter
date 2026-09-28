document.addEventListener("DOMContentLoaded", () => {
  // --------------------------------------------------------------------------
  // 1. File list data loading
  // --------------------------------------------------------------------------
  function loadFileList() {
    fetch("/list?dir=/")
      .then(response => response.json())
      .then(files => renderFileList(files))
      .catch(error => {
        console.error("Error loading file list:", error);
        renderMessage("Failed to load file system data.", true);
      });
  }

  // --------------------------------------------------------------------------
  // 2. File list rendering
  // --------------------------------------------------------------------------
  function renderFileList(files) {
    const tableBody = document.querySelector("#fileTable tbody");
    tableBody.innerHTML = "";

    if (files.length === 0) {
      renderMessage("No files found in directory.");
      return;
    }

    files.forEach(file => {
      const row = document.createElement("tr");
      row.appendChild(createTypeCell(file.type));
      row.appendChild(createNameCell(file.name));
      row.appendChild(createActionsCell(file));
      tableBody.appendChild(row);
    });
  }

  function renderMessage(message, isError = false) {
    const tableBody = document.querySelector("#fileTable tbody");
    tableBody.innerHTML = `<tr><td colspan="3" class="empty-message${isError ? " error-message" : ""}">${message}</td></tr>`;
  }

  function createTypeCell(type) {
    const cell = document.createElement("td");
    const badge = document.createElement("span");
    badge.className = `file-badge ${type}`;
    badge.textContent = type;
    cell.appendChild(badge);
    return cell;
  }

  function createNameCell(name) {
    const cell = document.createElement("td");
    cell.textContent = name;
    cell.style.fontWeight = "600";
    return cell;
  }

  function createActionsCell(file) {
    const cell = document.createElement("td");
    cell.className = "actions-cell";

    if (file.type === "file") {
      const deleteButton = document.createElement("button");
      deleteButton.className = "delete-button";
      deleteButton.type = "button";
      deleteButton.textContent = "DELETE";
      deleteButton.addEventListener("click", () => deleteFile(`/${file.name}`));
      cell.appendChild(deleteButton);
    } else {
      cell.textContent = "-";
      cell.style.color = "var(--muted)";
    }

    return cell;
  }

  // --------------------------------------------------------------------------
  // 3. File deletion
  // --------------------------------------------------------------------------
  function deleteFile(path) {
    if (!confirm(`Delete ${path}?`)) return;

    fetch(`/delete?file=${encodeURIComponent(path)}`)
      .then(response => {
        if (response.ok) {
          loadFileList();
          return;
        }

        return response.text().then(text => {
          throw new Error(text || "Delete failed");
        });
      })
      .catch(error => {
        console.error("Delete error:", error);
        alert(`Delete failed: ${error.message}`);
      });
  }

  // --------------------------------------------------------------------------
  // 4. Page initialization
  // --------------------------------------------------------------------------
  loadFileList();
});
