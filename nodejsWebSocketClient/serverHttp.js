var app = require('express')();
var bodyParser = require('body-parser');

app.use(bodyParser.json()); // for parsing application/json
app.use(bodyParser.urlencoded({ extended: true })); // for parsing application/x-www-form-urlencoded

app.post('/formtest2', function (req, res) {
  console.log(req.body);
  res.end();
});

app.post('/', function (req, res) {
  console.log("Packet from C server and controller\n");
  console.log(req.body);
  res.end('bye from httpServer');
});

app.listen(4000);