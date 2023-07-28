# Hướng dẫn sử dụng

Xin hãy đọc kĩ hướng dẫn này trước khi thao tác. Luôn phải có bản backup cho code của mình, nếu gặp vấn đề hoặc thấy dấu hiệu lạ, hãy tham khảo trợ giúp trên mạng hoặc từ người xung quanh.

Hướng dẫn này được dựa trên nhiều mẩu kinh nghiệm thu thập từ nhiều nguồn, hoạt động khác nhau. Do đó, nếu thấy có gì thiếu sót hoặc cảm thấy cần góp ý, đừng ngần ngại nêu ra ý kiến.

Nếu chưa từng sử dụng git/github, hãy tìm hiểu về những khái niệm cơ bản nhất của git để có thể nhanh chóng tiếp cận tài liệu này. Git là một công cụ lớn, nhưng để bắt đầu làm quen và sử dụng hằng ngày thì không cần biết quá nhiều.

Một số keyword cần quan tâm như:

1. Commit
2. Branch/tag
3. Pull
4. Push
5. Conflicts
6. Merge/pull request

Có rất nhiều tài nguyên ở nhiều dạng, từ text đến video để học git:

1. Sách ProGit: https://git-scm.com/book/en/v2 - Trước mắt quan tâm chương 2, 3, 5

## Về file gitignore

Trong thư mục project do Simplicity Studio tạo ra, có một vài thư mục không đáng hoặc không phù hợp để sao chép giữa các máy. Ví dụ như thư mục `autogen` là nơi chứa các file code do Sim tự tạo ra dựa vào thiết lập của các Software Component, hay như tập tin chứa các file output của quá trình build project...

Do đó tập tin `.gitignore` được thêm vào là để loại bỏ việc git theo dõi lại các file/thư mục trên. Tập tin này đã được chỉnh sửa và thử nghiệm sao cho git repo này chỉ chứa lượng thông tin vừa đủ khi import vào Simplicity Studio.

Đừng thấy lạ nếu thư mục trên Github thiếu đi một vài file hoặc thư mục có trên máy tính.

## Cách import project đã clone vào workspace của Simplicity Studio

## Hướng dẫn chung khi sử dụng repo này

Mỗi một project sẽ được đặt trong một thư mục của repo này. Những ai làm việc liên quan tới project nào thì sẽ chỉ nên thao tác trên thư mục của mình.

Để đơn giản hóa việc sử dụng git thì repo này sẽ chỉ có một branch duy nhất là master (hoặc main). Tất cả mọi người sẽ đều commit chung vào branch này.

Một khi code đã ổn định, hoạt động như mong muốn hoặc đạt một mục tiêu nào đó thì sẽ được tách sang branch khác để lưu lại. Sau đó việc thay đổi, sửa xóa, phát triển vẫn sẽ tiếp diễn trên branch chính.

## Hướng dẫn chi tiết về Workflow

Do có nhiều người cùng làm trên một nhánh, sẽ không tránh khỏi việc 

## Một số vấn đề và cách khắc phục

### How to resolve conflicts when pulling before committing.
