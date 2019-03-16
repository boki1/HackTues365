function filterTable () {
  let value;
  let input = document.getElementById("search-inp");
  let filter = input.value.toUpperCase();
  let table = document.getElementById("db-table");
  let tr = table.getElementsByTagName("tr");
  for (i = 0; i < tr.length; ++i) {
    td = tr[i].getElementsByTagName("td")[0];
    if (td) {
      value = td.textContent || td.innerText;
      if (value.toUpperCase().indexOf(filter) > -1) {
        tr[i].style.display = "";
      } else {
        tr[i].style.display = "none";
      }
    }       
  }
}
