#[test]
pub fn fizzbuzz() {
    cklein::run!(
        r#"
		for number in 1.to(100) {
			if number.mod(3) == 0 and number.mod(5) == 0 {
				print("FizzBuzz");
			} else if number.mod(3) == 0 {
				print("Fizz");
			} else if number.mod(5) == 0 {
				print("Buzz");
			} else {
				print(number);
			}
		}
	"#
    )
}
