var express = require('express')

var app = express()

app.post('/', (req, res) => {
    res.send('hello world')
    console.log('hello world')
})

app.listen(3000, () => {
    console.log('mulai di port 3000')
})