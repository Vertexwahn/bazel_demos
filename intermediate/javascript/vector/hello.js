/**
 * @fileoverview minimal test program that requires a third-party package from npm
 */
const acorn = require('acorn')
const result = JSON.stringify(acorn.parse('1', { ecmaVersion: 2020 })) + '\n'
if (process.argv[2]) {
    require('fs').writeFileSync(process.argv[2], result)
} else {
    process.stdout.write(result)
}
