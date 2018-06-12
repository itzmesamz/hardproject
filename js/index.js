var express = require('express')

var app = express()

app.get('/', (req, res) => {
    res.send(req.query)
    console.log(req.query)
})

app.listen(3000, () => {
    console.log('mulai di port 3000')
})