String parseHomePage();
String parseConfigurePage();
String parseControlPage();
String parseFeedingTimes(int NumberOfFeedings);

String footer = "<!-- Footer -->"
                "<footer class='w3-bottom w3-center w3-black w3-padding-small w3-opacity w3-hover-opacity-off'>"
                "<div class='w3-xlarge'>"
                "<a href='https://github.com/ZGoode/Tie-Fighter-Clock'><i class='fa fa-github w3-hover-opacity'></i></a>"
                "<a href='http://linkedin.com/in/zachary-goode-724441160'><i class='fa fa-linkedin w3-hover-opacity'></i></a>"
                "</div>"
                "</footer>"
                ""
                "</body>"
                "</html>";

String header = "<!DOCTYPE html>"
                "<html>"
                "<title>Food Dispenser</title>"
                "<meta charset='UTF-8'>"
                "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>"
                "<link rel='stylesheet' href='https://fonts.googleapis.com/css?family=Lato'>"
                "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>"
                "<style>"
                "body,h1,h2,h3,h4,h5,h6 {font-family: 'Lato', sans-serif;}"
                "body, html {"
                "height: 100%;"
                "color: #777;"
                "line-height: 1.8;"
                "}"
                "/* Create a Parallax Effect */"
                ".bgimg{"
                "background-attachment: fixed;"
                "background-position: center;"
                "background-repeat: no-repeat;"
                "background-size: cover;"
                "}"
                "/* Background Picture */"
                ".bgimg{"
                "background-image: url('%BACKGROUND_IMAGE%');"
                "min-height: 100%;"
                "}"
                ".w3-wide {letter-spacing: 10px;}"
                ".w3-hover-opacity {cursor: pointer;}"
                ".sidenav {"
                "height: 100%;"
                "width: 250px;"
                "position: fixed;"
                "z-index: 1;"
                "top: 0;"
                "left: 0;"
                "background-color: #111;"
                "overflow-x: hidden;"
                "padding-top: 20px;"
                "}"
                ".sidenav a {"
                "padding: 6px 8px 6px 16px;"
                "text-decoration: none;"
                "font-size: 25px;"
                "color: #818181;"
                "display: block;"
                "}"
                ".main {"
                "margin-left: 250px;"
                "padding: 0px 10px;"
                "}"
                ".checkboxes {"
                "vertical-align: bottom;"
                "position: relative;"
                "top: -17px;"
                "*overflow: hidden;"
                "}"
                "</style>"
                "<body>"
                "<!-- Navbar (sit on top) -->"
                "<div class='w3-top'>"
                "<div class='w3-bar' id='myNavbar'>"
                "<a href='Home' class='w3-bar-item w3-button'>HOME</a>"
                "<a href='Control' class='w3-bar-item w3-button w3-hide-small'><i class='fa fa-user'></i> CONTROL</a>"
                "<a href='Configure' class='w3-bar-item w3-button w3-hide-small'><i class='fa fa-cogs'></i> CONFIGURE</a>"
                "<a href='https://github.com/ZGoode/Tie-Fighter-Clock' class='w3-bar-item w3-button w3-hide-small'><i class='fa fa-th'></i> ABOUT</a>"
                "<a href='/WifiReset' class='w3-bar-item w3-button w3-hide-small w3-right w3-hover-red'>WIFI RESET</a>"
                "<a href='/FactoryReset' class='w3-bar-item w3-button w3-hide-small w3-right w3-hover-red'>FACTORY RESET</a>"
                "</div>"
                "</div>";

String homePage = "<!-- First Parallax Image with Logo Text -->"
                  "<div class='bgimg w3-display-container w3-opacity-min' id='home'>"
                  "<div class='w3-display-middle' style='white-space:nowrap;'>"
                  "<p><span class='w3-center w3-padding-large w3-black w3-xlarge w3-wide w3-animate-opacity'>Food<span class='w3-hide-small'> Dispenser</span></p>"
                  "</div>"
                  "</div>";

String configurePage = "<div class='bgimg w3-display-container w3-opacity-min' id='home'>"
                       "<div class='w3-display-middle' style='white-space:nowrap;'>"
                       "<div class='w3-black'>"
                       "<form class='w3-container' action='/updateConfig' method='get'><h2>Clock Config:</h2>"
                       "<p><label>User ID (for this interface)</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='userid' value='%USERID%' maxlength='20'></p>"
                       "<p><label>Password </label><input class='w3-input w3-border w3-margin-bottom' type='password' name='stationpassword' value='%STATIONPASSWORD%'></p>"
                       "<p><label>OTA Password </label><input class='w3-input w3-border w3-margin-bottom' type='password' name='otapassword' value='%OTAPASSWORD%'></p>"
                       "<p><label>Time Zone Offset </label><input class='w3-input w3-border w3-margin-bottom' type='text' name='timezone' value='%TIMEZONE%'></p>"
                       "<p><label>24 Hour Time </label><input class='w3-input w3-border w3-margin-bottom' type='checkbox' name='24hour' %24HOUR%></p>"
                       "<button class='w3-button w3-block w3-grey w3-section w3-padding' type='submit'>Save</button>"
                       "</form>"
                       "</div>"
                       "</div>"
                       "</div>";

String controlPage = "<div class='bgimg w3-display-container w3-opacity-min main' id='home'>"
                     "<div class='sidenav'>"
                     "<form class='w3-container' action='/updateControl' method='get'>"
                     "<h2>Feeding Config:</h2>"
                     "<label for='timeselection'>Select Feeding Time: </label><select id='timeselection' name='timeselection'>"
                     "%FEEDINGTIMEMENU%"
                     "</select>"
                     "<p><label>Hours : Minutes </label><input class='w3-input w3-border w3-margin-bottom' type='text' name='hours' value='0'><input class='w3-input w3-border w3-margin-bottom' type='text' name='minutes' value='0'>"
                     "<select id='ampm' name='ampm'><option value='1'>AM</option><option value='2'>PM</option></select></p>"
                     "<label>Monday </label><input class='w3-input w3-border w3-margin-bottom checkboxes' type='checkbox' name='monday'>"
                     "<label>Tuesday </label><input class='w3-input w3-border w3-margin-bottom checkboxes' type='checkbox' name='tuesday'>"
                     "<label>Wednesday </label><input class='w3-input w3-border w3-margin-bottom checkboxes' type='checkbox' name='wednesday'>"
                     "<label>Thursday </label><input class='w3-input w3-border w3-margin-bottom checkboxes' type='checkbox' name='thursday'>"
                     "<label>Friday </label><input class='w3-input w3-border w3-margin-bottom checkboxes' type='checkbox' name='friday'>"
                     "<label>Saturday </label><input class='w3-input w3-border w3-margin-bottom checkboxes' type='checkbox' name='saturday'>"
                     "<label>Sunday </label><input class='w3-input w3-border w3-margin-bottom checkboxes' type='checkbox' name='sunday'>"
                     "<label>Cups of Food: </label><input class='w3-input w3-border w3-margin-bottom' type='text' name='cupsoffood' value='0'></p>"
                     "<button class='w3-button w3-block w3-grey w3-section w3-padding' type='submit'>Save</button>"
                     "</form>"
                     "</div>"
                     "<div class='w3-display-middle' style='white-space:nowrap;'>"
                     "<div class='w3-black'>"
                     "%FEEDINGTIMES%"
                     "</div>"
                     "</div>"
                     "</div>";

String parseHomePage() {
  return header + homePage + footer;
}

String parseConfigurePage() {
  return header + configurePage + footer;
}

String parseControlPage() {
  return header + controlPage + footer;
}

String parseFeedingTimes(int NumberOfFeedings) {
  String temp = "";

  for (int i = 0; i < NumberOfFeedings; i++) {
    String temp2 = "<option value='";

    temp2.concat(i);
    temp2.concat("'>");
    temp2.concat((i + 1));
    temp2.concat("</option>");

    temp.concat(temp2);
  }

  temp.concat("<option value='");
  temp.concat(NumberOfFeedings);
  temp.concat("'>New</option>");

  return temp;
}
