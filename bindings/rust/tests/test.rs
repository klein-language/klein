#[test]
pub fn fizzbuzz() {
    let fizzbuzz = "
		for number in 1.to(100) {
			print(number);
		};
	";
    klein::run(fizzbuzz);
}
