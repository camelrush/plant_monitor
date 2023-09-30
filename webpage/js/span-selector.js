window.addEventListener("load", () => {
  var selector = document.getElementById("span-selector");
  selector.addEventListener("change", (event) => {
    reloadChart(event.target.value);
  });
  reloadChart("today");
});
